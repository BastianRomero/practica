/*****************************************************************************
 * EVT_BenchmarkHS_Sync.cpp
 *****************************************************************************
 *
 * Project:      EVT_BenchmarkHS_Sync
 * Description:  Example program
 *
 * (c) 2013      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_BenchmarkHS_Sync example opens all found EVT cameras and configures
 *   them, streams, and counts dropped frames and then calculates
 *   the effective frame rate. This single program creates one thread for each camera.
 *
 *   This example also intends to synchronize the exposures of all cameras with accuracy
 *   determined by how often the sync cycles are performed by adjusting the SYNC_ON_FRAME parameter.
 *
 *   !!!This example is currently only configured to sync two cameras!!!
 *   !!!This example requires Myricom's Sync NIC and an IRIGB source supplying
 *   !!!time into the Sync NIC IRIGB port.
 *
 *   Command line usage:
 *   EVT_BenchmarkHS_Sync <Frame Rate>
 *   ie.
 *   EVT_BenchmarkHS_Sync 100
 *   Specifying too high a frame rate uses the maximum that camera will support.
 *
 ****************************************************************************/

#include "stdafx.h"
#ifdef _MSC_VER
  #include <windows.h>
#else
  #include <pthread.h>
  #include <sys/time.h>
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
#define MAX_FRAMES 1000
#define FRAMES_BUFFERS 30

#define MAX_CAMERAS 10

#define SYNC_ON_FRAME 100

#define FR_VAL_DEF  60

#define SYNC_TOLERANCE_NS   10000 //10us. Used to determine how the offset calculations are performed
                                  //and thus who gets the positive offsets.

//Struct that includes all involving a camera such as CEmergentCamera objects, frames, etc.
struct worker
{
  CEmergentCamera       camera;
  CEmergentFrame        frames[FRAMES_BUFFERS];
  unsigned int          worker_id;
  unsigned long long    timestamp;
  int                   sync_offset;
#ifdef _MSC_VER
  HANDLE thread_id;
#else
  pthread_t  thread_id;
#endif
  bool                  done;
  unsigned int          frame_count;
  unsigned int          frame_to_recv;
  unsigned short        id_prev;
  unsigned int          dropped_frames;

};
typedef struct worker worker_t;

//Struct that has all system info including all workers.
struct G {
  worker_t              worker[MAX_CAMERAS];
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int          frame_rate;
  int                   frame_time;
  unsigned int          num_workers;
#ifdef _MSC_VER
  HANDLE wrk_mtx;
#else
  pthread_mutex_t wrk_mtx;
#endif	
} G;

void configure_defaults(CEmergentCamera* camera);
char* next_token;
void worker_thread(void *work_struct);
void synchronize_cameras(worker_t *p_wrk, CEmergentFrame *evtFrameRecv);

int main(int argc, char *argv[])
{
  unsigned int camera_count, camera_index;

  printf("----------------------------------------"); printf("\n");
  printf("EVT_BenchmarkHS_Sync : Example program  "); printf("\n");
  printf("----------------------------------------"); printf("\n");

  //Specifies frame rate on cmd line.
  if(argc == 2)
  {
    G.frame_rate = atoi(argv[1]);
    G.frame_time = 1000000000/G.frame_rate; //In ns.
  }

  //Find all cameras in system.
  unsigned int listcam_buf_size = MAX_CAMERAS;
  EVT_ListDevices(G.deviceInfo, &listcam_buf_size, &camera_count);

  if(camera_count<2)
  {
    printf("Enumerate Cameras: \tNeed at least 2 EVT cameras to run example. Exiting program.\n");
    return 0;
  }

  //Mutex used for opening the cameras.
#ifdef _MSC_VER
  G.wrk_mtx = CreateMutex(NULL, false, NULL);
#else
  if (pthread_mutex_init(&G.wrk_mtx, NULL) != 0)
  {
    printf("\n mutex init failed\n");
    return -1;
  }
#endif
  G.num_workers = camera_count;

  //Create threads for each camera.
  for(camera_index=0; camera_index<camera_count;camera_index++)
  {
    char* EVT_models[] = { "HS", "HT", "HR", "HB", "LR", "LB" };
    int EVT_models_count = sizeof(EVT_models) / sizeof(EVT_models[0]);
    bool is_EVT_camera = false;
    for(int i = 0; i < EVT_models_count; i++)
    {
        if(strncmp(G.deviceInfo[camera_index].modelName, EVT_models[i], 2) == 0)
        {
            is_EVT_camera = true;
            break; //it is an EVT camera
        }
    }
    if(is_EVT_camera)
    {
      G.worker[camera_index].done = false;
      G.worker[camera_index].frame_count = 0;
      G.worker[camera_index].frame_to_recv = MAX_FRAMES;
      G.worker[camera_index].id_prev = 0;
      G.worker[camera_index].dropped_frames = 0;
      G.worker[camera_index].worker_id = camera_index;
      G.worker[camera_index].timestamp = 0;

#ifdef _MSC_VER
    G.worker[camera_index].thread_id = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&worker_thread, (void*)(&G.worker[camera_index]), 0, NULL);
#else
    pthread_create(&G.worker[camera_index].thread_id, NULL, (void* (*)(void*))&worker_thread, (void*)(&G.worker[camera_index]));
#endif
    }
  }

  //Wait for threads to finish.
  for(camera_index=0;camera_index<camera_count;camera_index++)
  {
#ifdef _MSC_VER
    WaitForSingleObject(G.worker[camera_index].thread_id, INFINITE);
#else
    pthread_join(G.worker[camera_index].thread_id, NULL);
#endif
  }
    
  printf("\nMain Program Done.\n");

  return(0);
}

void worker_thread(void *work_struct)
{
  EVT_ERROR err = EVT_SUCCESS;
  bool buffer_used = false, buffer_recd = false;
  unsigned int frame_rate_max, frame_rate_min, frame_rate;
  worker_t *p_wrk;
  CEmergentCamera* p_cam;
  CEmergentFrame *p_frms;
  CEmergentFrame evtFrameRecv;
  p_wrk  = (worker_t *)work_struct;
  p_cam  = &(p_wrk->camera);
  p_frms = p_wrk->frames;

  //Option for setting CPU for this camera thread.
//  SetThreadIdealProcessor(GetCurrentThread(), p_wrk->worker_id);

  //Lock the Mutex
#ifdef _MSC_VER
  WaitForSingleObject(G.wrk_mtx, INFINITE);
#else
  pthread_mutex_lock(&G.wrk_mtx);
#endif

  err = EVT_CameraOpen(p_cam, &(G.deviceInfo[p_wrk->worker_id]));
  if(err) 
  {
    printf("Open Camera %d: \t\tError. Exiting thread.\n", p_wrk->worker_id);
    return;
  }
  else
  {
    printf("Open Camera %d: \t\tCamera Opened\n", p_wrk->worker_id);
  }

	//Prepare host side for streaming.
	err = EVT_CameraOpenStream(p_cam);
  if(err) printf("EVT_CameraOpenStream: Error\n");

  //Allocate buffers and queue up frames before entering grab loop.
	for(int frame_count=0;frame_count<FRAMES_BUFFERS;frame_count++)
	{
    //Three params used for memory allocation. Worst case covers all models so no recompilation required.
    p_frms[frame_count].size_x = 4096;
    p_frms[frame_count].size_y = 3072;
    p_frms[frame_count].pixel_type = GVSP_PIX_MONO8;

    err = EVT_AllocateFrameBuffer(p_cam, &p_frms[frame_count], EVT_FRAME_BUFFER_ZERO_COPY);
    if(err) printf("EVT_AllocateFrameBuffer Error!\n");

    err = EVT_CameraQueueFrame(p_cam, &p_frms[frame_count]);
    if(err) printf("EVT_CameraQueueFrame Error!\n");
	}

  //Unlock the Mutex
#ifdef _MSC_VER
  ReleaseMutex(G.wrk_mtx);
#else
  pthread_mutex_unlock(&G.wrk_mtx);
#endif

  //To avoid conflict with settings in other examples.
  configure_defaults(p_cam);

  //Specifies frame rate
  frame_rate = G.frame_rate; //From cmd line. Could be overwritten below.

  EVT_CameraGetUInt32ParamMax(p_cam, "FrameRate",   &frame_rate_max);
  EVT_CameraGetUInt32ParamMin(p_cam, "FrameRate",   &frame_rate_min);
  if(frame_rate >= frame_rate_min && frame_rate <= frame_rate_max)
  {
    EVT_CameraSetUInt32Param(p_cam, "FrameRate", frame_rate);
    printf("FrameRate Set: \t\t%d\n", frame_rate);
  }
  else //Set to max.
  {
    EVT_CameraSetUInt32Param(p_cam, "FrameRate", frame_rate_max);
    frame_rate = frame_rate_max;
    printf("FrameRate Set: \t\t%d\n", frame_rate);
  }
  
	//Tell camera to start streaming
	err = EVT_CameraExecuteCommand(p_cam, "AcquisitionStart");
  if(err) printf("EVT_CameraExecuteCommand: Error\n");

  //////////////////////
  //Grab frame loop
  while (!p_wrk->done)
  {
    //Re-queue frame if we have received one which is done being processed.
    if(buffer_used)
    {
      //Re-queue. Still need to re-queue if dropped frame due to frame_id check.
      err = EVT_CameraQueueFrame(p_cam, &evtFrameRecv);
      if(err) printf("EVT_CameraQueueFrame Error!\n");
      buffer_used = false;
    }

    //Get the next buffer.
    err = EVT_CameraGetFrame(p_cam, &evtFrameRecv, EVT_INFINITE);
    if(!err)
    {
      //Counting dropped frames through frame_id as redundant check.
      if(((evtFrameRecv.frame_id) != p_wrk->id_prev+1) && (p_wrk->frame_count != 0)) //Ignore very first frame as id is unknown.
      {
        p_wrk->dropped_frames++;
        buffer_recd = false;
      }
      else
      {
        p_wrk->frame_count++;
        buffer_recd = true;
      }
    }
    else
    {
      p_wrk->dropped_frames++;
      buffer_recd = false;
    }

    //In GVSP there is no id 0 so when 16 bit id counter in camera is max then the next id is 1 so set prev id to 0 for math above.
    if(evtFrameRecv.frame_id == 65535)
      p_wrk->id_prev = 0;
    else
      p_wrk->id_prev = evtFrameRecv.frame_id;

    //Indicate buffer has been used so at top of while we can re-queue.
    buffer_used = true;

    //Check if we have received ALL frames amongst worker threads.
    if (p_wrk->frame_count >= p_wrk->frame_to_recv)
      p_wrk->done = true;

    //Don't process further if frame dropped.
    if(!buffer_recd) continue;


    //Sync cycle code.
    synchronize_cameras(p_wrk, &evtFrameRecv);
    
    //Now can process the frame here.
    //evtFrameRecv has frame data....

  }
  
	//Tell camera to stop streaming
  EVT_CameraExecuteCommand(p_cam, "AcquisitionStop");

  //Release frame buffers
	for(int frame_count=0;frame_count<FRAMES_BUFFERS;frame_count++)
	{
		EVT_ReleaseFrameBuffer(p_cam, &p_frms[frame_count]);
	}
  
	//Host side tear down for stream.
	EVT_CameraCloseStream(p_cam);

  //To avoid conflict with settings in other examples.
  configure_defaults(p_cam);

	EVT_CameraClose(p_cam);

  printf("\n");
  printf("Camera %d Stats:\n", p_wrk->worker_id);
  printf("Images Captured: \t%d\n", p_wrk->frame_count);
  printf("Dropped Frames: \t%d\n", p_wrk->dropped_frames);

  printf("Close Camera %d: \tCamera Closed\n", p_wrk->worker_id);
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

void synchronize_cameras(worker_t *p_wrk, CEmergentFrame *evtFrameRecv)
{
  unsigned int i;

  //Assign all worker's timestamps on each frame received.
  p_wrk->timestamp = evtFrameRecv->nsecs;

  //Calculate preliminary offset in timestamps.
  long long prelim_offset = ((long long)G.worker[0].timestamp - (long long)G.worker[1].timestamp);

  //0th worker will determine all worker's offsets including itself.
  if(p_wrk->worker_id == 0 && (prelim_offset > -2*G.frame_time && prelim_offset < 2*G.frame_time))
  {
    //Analyze offsets when 0th worker hits every SYNC_ON_FRAME frames(as an example).
    if(G.worker[0].frame_count % SYNC_ON_FRAME == 1)
    {
      //Defaults. Changed below if non-zero offset required.
      G.worker[0].sync_offset = 0;
      G.worker[1].sync_offset = 0;

      //Deal now with the cases...
      if((prelim_offset > 0) && (prelim_offset < (G.frame_time - SYNC_TOLERANCE_NS))) //Region 1: positive offset which will be applied to worker
      {
        //1st worker takes prelim_offset
        G.worker[1].sync_offset = (int)prelim_offset;
      }
      else if(prelim_offset >= (G.frame_time - SYNC_TOLERANCE_NS)) //Region 2/3: positive offset and still close to sync but one frame time out.
      {
        //Two cases here.
        if(prelim_offset > G.frame_time)  //Region 2.
        {
          //Now makes a small positive offset for camera 1 or big for 1st time through. Perhaps this is not right to have big.
          prelim_offset = prelim_offset - G.frame_time;

          //1st worker takes frame time subtracted prelim_offset.
          G.worker[1].sync_offset = (int)prelim_offset;
        }
        else // Region 3.
        {
          //Now makes a small positive offset for camera 0(what would have been negative for Camera 1).
          prelim_offset = G.frame_time - prelim_offset;

          //Camera 0 takes frame time subtracted prelim_offset.
          G.worker[0].sync_offset = (int)prelim_offset;
        }
      }
      else if(prelim_offset < 0) //Rare case. Region 4.
      {
        //Camera 0 takes small positive prelim_offset.
        G.worker[0].sync_offset = -((int)prelim_offset);
      }

      printf("\nCamera Sync Cycle...\n");
      printf("Before Sync...\n");

      for(i=0;i<G.num_workers;i++)
      {
        printf("Cam %d: OFFSET = %d\n", G.worker[i].worker_id, G.worker[i].sync_offset);
      }

      //Now tell cameras/workers to use the offset(extends the workers current frame times by this positive offset)
      for(i=0;i<G.num_workers;i++)
      {
        if(G.worker[i].sync_offset != 0)
        {
          G.worker[i].camera.SetGvcpReply(false);

          EVT_CameraSetUInt32Param(&(G.worker[i].camera), "SyncOffset", (G.worker[i].sync_offset)/20); // /20 is since 20ns camera clk period. 
          EVT_CameraExecuteCommand(&(G.worker[i].camera), "Sync");

          G.worker[i].camera.SetGvcpReply(true);
        }
      }
    }

    //Post sync updates.
    if(G.worker[0].frame_count % SYNC_ON_FRAME == 5) //Report sync accuracy 5 frames later - a few frames after sync performed.
    {
      printf("After Sync...\n");

      for(i=0;i<G.num_workers;i++)
      {
        int offset = (int)(G.worker[0].timestamp - G.worker[i].timestamp);
        if(offset < 0) offset = -offset; //Absolute value of the offset.
        printf("Cam %d: OFFSET = %d\n", G.worker[i].worker_id, offset);
      }
    }
  }
}
