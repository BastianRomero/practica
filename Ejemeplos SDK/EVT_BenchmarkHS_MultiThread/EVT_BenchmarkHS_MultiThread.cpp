/*****************************************************************************
 * EVT_BenchmarkHS_MultiThread.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_BenchmarkHS_MultiThread
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_BenchmarkHS_MultiThread example opens the camera and configures
 *   it for highest speed and counts dropped frames and then calculates
 *   the effective frame rate. 0 dropped frames is expected.
 *   The example can also perform calculations and uses multiple CPUs
 *   to handle the frames in turn for load balancing.
 *   Command line usage:
 *   EVT_BenchmarkHS_MultiThread <camera=0,1,2,...> <PixelFormat>
 *   ie.
 *   EVT_BenchmarkHS_MultiThread 0 Mono8
 *   EVT_BenchmarkHS_MultiThread 1 BayerGB8
 ****************************************************************************/

#include "stdafx.h"
#ifdef _MSC_VER
  #include <windows.h>
#else
  #include <pthread.h>
  #include <sys/time.h>
  #include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>

using namespace std;
using namespace Emergent;

#define SUCCESS 0
//#define XML_FILE   "C:\\xml\\Emergent_HS-2000-M_1_0.xml"
#define MAX_FRAMES 1000
#define FRAMES_BUFFERS 30

#define MAX_CAMERAS 10

void configure_defaults(CEmergentCamera* camera);
char* next_token;
void work_thread(void *work_struct);

#define MAX_WORKERS 16

struct worker 
{
#ifdef _MSC_VER
  HANDLE thread_id;
#else
	pthread_t  thread_id;
#endif
  int    worker_id;
};
typedef struct worker worker_t;

struct G {
#ifdef _MSC_VER
  HANDLE wrk_mtx;
#else
	pthread_mutex_t wrk_mtx;
#endif	
  worker_t worker[MAX_WORKERS];
  bool done;
  unsigned int frame_count;
  unsigned int processed_frame_count;
  unsigned int frame_to_recv;
  unsigned int frame_id_prev;
  CEmergentCamera *p_camera;
  unsigned short id_prev;
  unsigned int dropped_frames;
} G;

int main(int argc, char *argv[])
{
  CEmergentCamera camera;
  int ReturnVal = SUCCESS;
  unsigned int frame_rate_max, frame_rate_min, frame_rate_inc, frame_rate;
  unsigned int height_max, width_max;
  CEmergentFrame evtFrame[FRAMES_BUFFERS], evtFrameRecv;
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;
  int cameras_found = 0;
  float test_time;

  printf("-----------------------------------------------"); printf("\n");
  printf("EVT_BenchmarkHS_MultiThread : Example program  "); printf("\n");
  printf("-----------------------------------------------"); printf("\n");

  //Find all cameras in system.
  unsigned int listcam_buf_size = MAX_CAMERAS;
  EVT_ListDevices(deviceInfo, &listcam_buf_size, &count);
  if(count==0)
  {
    printf("Enumerate Cameras: \tNo cameras found. Exiting program.\n");
    return 0;
  }

  //Find and use first EVT camera.
  for(camera_index=0; camera_index<MAX_CAMERAS;camera_index++)
  {
    char* EVT_models[] = { "HS", "HT", "HR", "HB", "LR", "LB" };
    int EVT_models_count = sizeof(EVT_models) / sizeof(EVT_models[0]);
    bool is_EVT_camera = false;
    for(int i = 0; i < EVT_models_count; i++)
    {
        if(strncmp(deviceInfo[camera_index].modelName, EVT_models[i], 2) == 0)
        {
            is_EVT_camera = true;
            break; //it is an EVT camera
        }
    }
    if(is_EVT_camera)
    {
      cameras_found++;
      if(argc == 1) break; //Default with no args is use the first camera.
    }

    if(cameras_found == ((atoi(argv[1])) + 1))
    {
      break; //Found EVT camera so break. i carries index for open.
    }
    
    if(camera_index==(MAX_CAMERAS-1))
    {
      printf("No EVT cameras found. Exiting program\n");
      return 0;
    }
  }  

  //Create a mutex to allow exclusive access to resources for getting frames in worker threads.
#ifdef _MSC_VER
	G.wrk_mtx = CreateMutex(NULL, false, NULL);
#else
  if (pthread_mutex_init(&G.wrk_mtx, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return -1;
    }
#endif

  //Open the camera. Example usage. Camera found needs to match XML.
#ifdef XML_FILE
  ReturnVal = EVT_CameraOpen(&camera, &deviceInfo[camera_index], XML_FILE);
#else
  ReturnVal = EVT_CameraOpen(&camera, &deviceInfo[camera_index]);      
#endif
  if(ReturnVal != SUCCESS)
  {
    printf("Open Camera: \t\tError. Exiting program.\n");
    return ReturnVal;
  }
  else
  {
    printf("Open Camera: \t\tCamera Opened\n\n");
  }

  //To avoid conflict with settings in other examples.
  configure_defaults(&camera);

  //Get resolution.
  EVT_CameraGetUInt32ParamMax(&camera, "Height", &height_max);
  EVT_CameraGetUInt32ParamMax(&camera, "Width" , &width_max);
  printf("Resolution: \t\t%d x %d\n", width_max, height_max);

  //Specifies pixel format
  if(argc >= 3)
  {
    printf("PixelFormat: \t\t%s\n", argv[2]);
    ReturnVal = EVT_CameraSetEnumParam(&camera, "PixelFormat", argv[2]);
    if(ReturnVal != SUCCESS)
    {
      printf("EVT_CameraSetEnumParam: PixelFormat Error\n");
	    EVT_CameraClose(&camera);
      printf("\nClose Camera: \t\tCamera Closed\n");
      return ReturnVal;
    }
  }

  //Frame rate
  EVT_CameraGetUInt32ParamMax(&camera, "FrameRate", &frame_rate_max);
  printf("FrameRate Max: \t\t%d\n", frame_rate_max);
  EVT_CameraGetUInt32ParamMin(&camera, "FrameRate", &frame_rate_min);
  printf("FrameRate Min: \t\t%d\n", frame_rate_min);
  EVT_CameraGetUInt32ParamInc(&camera, "FrameRate", &frame_rate_inc);
  printf("FrameRate Inc: \t\t%d\n", frame_rate_inc);

  frame_rate = frame_rate_max;
  EVT_CameraSetUInt32Param(&camera, "FrameRate", frame_rate);
  printf("FrameRate Set: \t\t%d\n", frame_rate);

	//Prepare host side for streaming.
	ReturnVal = EVT_CameraOpenStream(&camera);
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraOpenStream: Error\n");
    return ReturnVal;
  }

  //Allocate buffers and queue up frames before entering grab loop.
	for(int frame_count=0;frame_count<FRAMES_BUFFERS;frame_count++)
	{
    //Three params used for memory allocation. Worst case covers all models so no recompilation required.
    evtFrame[frame_count].size_x = width_max;
    evtFrame[frame_count].size_y = height_max;

	if (argc >= 3)
	{
		if (strcmp(argv[2], "Mono8") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8;
		}
		else if (strcmp(argv[2], "Mono10") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_MONO10;
		}
		else if (strcmp(argv[2], "Mono10Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_MONO10_PACKED;
		}
		else if (strcmp(argv[2], "Mono12") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_MONO12;
		}
		else if (strcmp(argv[2], "Mono12Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_MONO12_PACKED;
		}
		else if (strcmp(argv[2], "BayerGB8") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB8;
		}
		else if (strcmp(argv[2], "BayerGR8") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR8;
		}
		else if (strcmp(argv[2], "BayerRG8") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG8;
		}
		else if (strcmp(argv[2], "BayerBG8") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG8;
		}
		else if (strcmp(argv[2], "BayerGB10") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB10;
		}
		else if (strcmp(argv[2], "BayerGR10") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR10;
		}
		else if (strcmp(argv[2], "BayerRG10") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG10;
		}
		else if (strcmp(argv[2], "BayerBG10") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG10;
		}
		else if (strcmp(argv[2], "BayerGB12") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB12;
		}
		else if (strcmp(argv[2], "BayerGR12") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR12;
		}
		else if (strcmp(argv[2], "BayerRG12") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG12;
		}
		else if (strcmp(argv[2], "BayerBG12") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG12;
		}
		else if (strcmp(argv[2], "RGB8Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_RGB8;
		}
		else if (strcmp(argv[2], "RGB10Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_RGB10;
		}
		else if (strcmp(argv[2], "RGB12Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_RGB12;
		}
		else if (strcmp(argv[2], "BGR8Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BGR8;
		}
		else if (strcmp(argv[2], "BGR10Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BGR10;
		}
		else if (strcmp(argv[2], "BGR12Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_BGR12;
		}
		else if (strcmp(argv[2], "YUV411Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_YUV411_PACKED;
		}
		else if (strcmp(argv[2], "YUV422Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_YUV422_PACKED;
		}
		else if (strcmp(argv[2], "YUV444Packed") == 0)
		{
			evtFrame[frame_count].pixel_type = GVSP_PIX_YUV444_PACKED;
		}
	}
	else //Good for default case with no cmd line params which covers color and mono as same size bytes/pixel.
	{    //Note that these settings are used for memory alloc only.
		evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8;
	}

    err = EVT_AllocateFrameBuffer(&camera, &evtFrame[frame_count], EVT_FRAME_BUFFER_ZERO_COPY);
    if(err) printf("EVT_AllocateFrameBuffer Error!\n");
    err = EVT_CameraQueueFrame(&camera, &evtFrame[frame_count]);
    if(err) printf("EVT_CameraQueueFrame Error!\n");
	}

  

  //Get info for number of processors in the system, et al.
	unsigned int dwNumberOfProcessors;
#ifdef _MSC_VER
  SYSTEM_INFO info;
  GetSystemInfo(&info);
	dwNumberOfProcessors = info.dwNumberOfProcessors;
#else
	dwNumberOfProcessors = sysconf( _SC_NPROCESSORS_ONLN );
#endif

    printf("dwNumberOfProcessors %d\n", dwNumberOfProcessors);

  //Not done before starting threads.
  G.done = false;
  G.frame_count = 0;
  G.processed_frame_count = 0;
  G.frame_to_recv = MAX_FRAMES;
  G.id_prev = 0;
  G.dropped_frames = 0;

  //Start as many worker threads as we have processors.
  for(unsigned int i=0; i < dwNumberOfProcessors; i++) 
  {
    G.worker[i].worker_id = i;
    G.p_camera = &camera;
#ifdef _MSC_VER
    G.worker[i].thread_id = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&work_thread, (void*)(&G.worker[i]), 0, NULL);
#else
	pthread_create(&G.worker[i].thread_id, NULL, (void* (*)(void*))&work_thread, (void*)(&G.worker[i]));
#endif
  }

	//Tell camera to start streaming
	ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraExecuteCommand: Error\n");
    return ReturnVal;
  }

#ifdef _MSC_VER
  LARGE_INTEGER tick_begin, tick_end, hp_counter_freq;
  QueryPerformanceFrequency(&hp_counter_freq); //Get high perf counter freq in counts per second.
  QueryPerformanceCounter(&tick_begin); //Start timer
  //Wait for threads to finish.
  for(unsigned int i = 0; i < dwNumberOfProcessors; i++)
    WaitForSingleObject(G.worker[i].thread_id, INFINITE);

  QueryPerformanceCounter(&tick_end); //End timer

  test_time = (float)(tick_end.QuadPart-tick_begin.QuadPart)/(float)(hp_counter_freq.QuadPart);
#else
	timeval startTime, endTime;
	gettimeofday(&startTime, NULL);
	//Wait for threads to finish.
	for(unsigned int i = 0; i < dwNumberOfProcessors; i++)
	{
		pthread_join(G.worker[i].thread_id, NULL);
	}

	gettimeofday(&endTime, NULL);
	test_time = (endTime.tv_sec  - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000000.0;
#endif


	//Tell camera to stop streaming
  EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

  //Report stats
  printf("\n\n");
  printf("Images Captured: \t%d\n", G.frame_count);
  printf("Dropped Frames: \t%d\n", G.dropped_frames);
  printf("Frame Rate Setting: \t%d\n", frame_rate);
  printf("Calculated Frame Rate: \t%f\n", G.frame_count/test_time);


  //Release frame buffers
	for(int frame_count=0;frame_count<FRAMES_BUFFERS;frame_count++)
	{
		EVT_ReleaseFrameBuffer(&camera, &evtFrame[frame_count]);
	}

	//Host side tear down for stream.
	EVT_CameraCloseStream(&camera);

  //To avoid conflict with settings in other examples.
  configure_defaults(&camera);

	EVT_CameraClose(&camera);
  printf("\nClose Camera: \t\tCamera Closed\n");

  return(0);
} 

//A function to set all appropriate defaults so 
//running other examples does not require reconfiguration.
void configure_defaults(CEmergentCamera* camera)
{
  unsigned int width_max, height_max, param_val_max;
  const unsigned long enumBufferSize = 1000;
  unsigned long enumBufferSizeReturn = 0;
  char enumBuffer[enumBufferSize];

  //Order is important as param max/mins get updated.
  EVT_CameraGetEnumParamRange(camera, "PixelFormat", enumBuffer, enumBufferSize, &enumBufferSizeReturn);
  char* enumMember = strtok_s(enumBuffer, ",", &next_token);
  EVT_CameraSetEnumParam(camera,      "PixelFormat", enumMember);

  EVT_CameraSetUInt32Param(camera,    "FrameRate", 30);

  EVT_CameraSetUInt32Param(camera,    "OffsetX", 0);
  EVT_CameraSetUInt32Param(camera,    "OffsetY", 0);

  EVT_CameraGetUInt32ParamMax(camera, "Width", &width_max);
  EVT_CameraSetUInt32Param(camera,    "Width", width_max);

  EVT_CameraGetUInt32ParamMax(camera, "Height", &height_max);
  EVT_CameraSetUInt32Param(camera,    "Height", height_max);

  EVT_CameraSetEnumParam(camera,      "AcquisitionMode",        "Continuous");
  EVT_CameraSetUInt32Param(camera,    "AcquisitionFrameCount",  1);
  EVT_CameraSetEnumParam(camera,      "TriggerSelector",        "AcquisitionStart");
  EVT_CameraSetEnumParam(camera,      "TriggerMode",            "Off");
  EVT_CameraSetEnumParam(camera,      "TriggerSource",          "Software");
  EVT_CameraSetEnumParam(camera,      "BufferMode",             "Off");
  EVT_CameraSetUInt32Param(camera,    "BufferNum",              0);

  EVT_CameraGetUInt32ParamMax(camera, "GevSCPSPacketSize", &param_val_max);
  EVT_CameraSetUInt32Param(camera,    "GevSCPSPacketSize", param_val_max);

  EVT_CameraSetUInt32Param(camera,    "Gain", 256);
  EVT_CameraSetUInt32Param(camera,    "Offset", 0);

  EVT_CameraSetBoolParam(camera,      "LUTEnable", false);
  EVT_CameraSetBoolParam(camera,      "AutoGain", false);
}

void work_thread(void *work_struct)
{
  worker_t *wrk;
  wrk = (worker_t *)work_struct;

  int ReturnVal = SUCCESS;
  CEmergentFrame evtFrameRecv;
  bool buffer_used = false, buffer_recd = false;
  unsigned int frame_count_this = 0;

  printf("Worker Thread %d Started...\n", wrk->worker_id);

#ifdef _MSC_VER
  SetThreadIdealProcessor(GetCurrentThread(), wrk->worker_id);
#else
	cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(wrk->worker_id, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif

  while (!G.done) 
  {
    //Lock the frame resource.
#ifdef _MSC_VER
    WaitForSingleObject(G.wrk_mtx, INFINITE);
#else
	pthread_mutex_lock(&G.wrk_mtx);
#endif
    if(G.done)
	{
#ifdef _MSC_VER
    ReleaseMutex(G.wrk_mtx);
#else
	pthread_mutex_unlock(&G.wrk_mtx);
#endif
	 break;
	}

    //Re-queue frame if we have received one which is done being processed.
    if(buffer_used) 
    {
      //Re-queue. Still need to re-queue if dropped frame due to frame_id check.
      ReturnVal = EVT_CameraQueueFrame(G.p_camera, &evtFrameRecv); 
      if(ReturnVal) printf("EVT_CameraQueueFrame Error!\n");
      buffer_used = false;
    }
    //Get the next buffer.
    ReturnVal = EVT_CameraGetFrame(G.p_camera, &evtFrameRecv, EVT_INFINITE);
    if(!ReturnVal)
    {
      //Counting dropped frames through frame_id as redundant check. 
      if(((evtFrameRecv.frame_id) != G.id_prev+1) && (G.frame_count != 0)) //Ignore very first frame as id is unknown.
      {
        G.dropped_frames++;
        buffer_recd = false;
        //printf("\ndfe!\n");
        //printf("%d %d %d\n", evtFrameRecv.frame_id, G.id_prev, G.frame_count);
      }
      else
      {
        G.frame_count++;
        buffer_recd = true;
      }
    }
    else
    {
      G.dropped_frames++;
      buffer_recd = false;
    }

    //In GVSP there is no id 0 so when 16 bit id counter in camera is max then the next id is 1 so set prev id to 0 for math above.
    if(evtFrameRecv.frame_id == 65535)
      G.id_prev = 0;
    else
      G.id_prev = evtFrameRecv.frame_id;

    //Indicate buffer has been used so at top of while we can re-queue.
    buffer_used = true;

    //Check if we have received ALL frames amongst worker threads.
    if (G.frame_count >= G.frame_to_recv)
      G.done = true;

    //Others will change this when outside mtx so save it for AVI sake.
    frame_count_this = G.frame_count;
    //Unlock the frame resource so other workers can access.
#ifdef _MSC_VER
    ReleaseMutex(G.wrk_mtx);
#else
	pthread_mutex_unlock(&G.wrk_mtx);
#endif
    //Don't process further if frame dropped.
    if(!buffer_recd) continue;

    //Process the block here. Sleep simulates a processing time.
    //Sleep(10);

    //Wait for threads turn to process. Helps keep frames in order for process.
    while(frame_count_this != G.processed_frame_count + 1);

    //Incrementing this counter will allow next thread to do its processing.
    G.processed_frame_count++;

    //A little printf progress meter.
    if(G.frame_count % 100 == 99) 
    {
      printf(".");
#ifndef _MSC_VER 
      fflush(stdout);
#endif
    }

  }

  printf("\nWorker Thread %d Ended...", wrk->worker_id);
}

