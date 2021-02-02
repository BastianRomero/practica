/*****************************************************************************
 * EVT_BenchmarkHS.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_BenchmarkHS
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_BenchmarkHS example opens the camera and configures
 *   it for highest speed and counts dropped frames and then calculates
 *   the effective frame rate. 0 dropped frames is expected.
 *   This program can be invoked multiple times in seperate DOS boxes
 *   to run on multiple cameras simulteneously.
 *   Command line usage:
 *   EVT_BenchmarkHS <camera=0,1,2,...> <PixelFormat> <CPU>
 *   ie.
 *   EVT_BenchmarkHS 0 Mono8 1
 *   ie. running multiple times in seperate DOS boxes and each on different CPUs....
 *   Script 1:
 *   EVT_BenchmarkHS 0 Mono8 1
 *   Script 2:
 *   EVT_BenchmarkHS 1 BayerGB8 2
 ****************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
  #include <windows.h>
#else
  #include <pthread.h>
  #include <sys/time.h>
#endif
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
int cpuToUse;

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

  printf("-------------------------------"); printf("\n");
  printf("BenchmarkHS : Example program  "); printf("\n");
  printf("-------------------------------"); printf("\n");

#ifdef _MSC_VER  
  //Set the CPU to run on if specified on command line.
  if(argc == 4)
  {
    cpuToUse = atoi(argv[3]); // 1 the first cpu, 2 the second cpu
    if(cpuToUse > 0)
    {
      
      DWORD_PTR cpuMask = static_cast<unsigned __int64>(1) << (cpuToUse - 1);
      DWORD_PTR oldmask = ::SetThreadAffinityMask(::GetCurrentThread(), cpuMask);
      if(!oldmask)
      {
        printf("Set CPU: \tError setting CPU. Exiting program.\n");
        return 0;
      }
      else
        printf("Setting CPU to %d.\n", cpuToUse);
    }
  }
#else
  //Set the CPU to run on if specified on command line.
  if(argc == 4)
  {
    cpuToUse = atoi(argv[3]); // 1 the first cpu, 2 the second cpu
    if(cpuToUse > 0)
    {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(cpuToUse-1, &cpuset);
      pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
      printf("Setting CPU to %d.\n", cpuToUse);
    }
  }
#endif

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
    char* EVT_models[] = {"HS", "HT", "HR", "HB", "LR", "LB" };
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
//Mono camera pixel formats
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "Mono8");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "Mono10");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "Mono10Packed");
//Color camera pixel formats
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BayerGB8");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BayerGR8");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BayerRG8");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BayerGB10");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BayerGR10");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BayerRG10");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "RGB8Packed");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "RGB10Packed");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BGR8Packed");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "BGR10Packed");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "YUV411Packed");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "YUV422Packed");
//  EVT_CameraSetEnumParam(&camera,      "PixelFormat", "YUV444Packed");

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
  printf("abriendo stream \n");
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

    if(argc >= 3)
    {
      if(strcmp(argv[2], "Mono8") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8;
      }
      else if(strcmp(argv[2], "Mono10") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_MONO10;
      }
      else if(strcmp(argv[2], "Mono10Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_MONO10_PACKED;
      }
	  else if(strcmp(argv[2], "Mono12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_MONO12;
	  }
	  else if(strcmp(argv[2], "Mono12Packed") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_MONO12_PACKED;
	  }
      else if(strcmp(argv[2], "BayerGB8") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB8;
      }
      else if(strcmp(argv[2], "BayerGR8") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR8;
      }
      else if(strcmp(argv[2], "BayerRG8") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG8;
      }
	  else if(strcmp(argv[2], "BayerBG8") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG8;
	  }
	  else if(strcmp(argv[2], "BayerGB10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB10;
	  }
	  else if(strcmp(argv[2], "BayerGR10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR10;
	  }
	  else if(strcmp(argv[2], "BayerRG10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG10;
	  }
	  else if(strcmp(argv[2], "BayerBG10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG10;
	  }
	  else if(strcmp(argv[2], "BayerGB12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB12;
	  }
	  else if(strcmp(argv[2], "BayerGR12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR12;
	  }
	  else if(strcmp(argv[2], "BayerRG12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG12;
	  }
	  else if(strcmp(argv[2], "BayerBG12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG12;
	  }
      else if(strcmp(argv[2], "RGB8Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_RGB8;
      }
      else if(strcmp(argv[2], "RGB10Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_RGB10;
      }
	  else if(strcmp(argv[2], "RGB12Packed") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_RGB12;
	  }
      else if(strcmp(argv[2], "BGR8Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_BGR8;
      }
      else if(strcmp(argv[2], "BGR10Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_BGR10;
      }
	  else if(strcmp(argv[2], "BGR12Packed") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BGR12;
	  }
      else if(strcmp(argv[2], "YUV411Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_YUV411_PACKED;
      }
      else if(strcmp(argv[2], "YUV422Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_YUV422_PACKED;
      }
      else if(strcmp(argv[2], "YUV444Packed") == 0)
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

//  printf("Pixel Type: \t\t%8.8x\n", evtFrame[0].pixel_type);

	//Tell camera to start streaming
	ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraExecuteCommand: Error\n");
    return ReturnVal;
  }

  printf("Grabbing Frames...\n");

  //Start timer
#ifdef _MSC_VER 
  clock_t begin=clock();
#else
  timeval startTime, endTime;
  gettimeofday(&startTime, NULL);
#endif

  unsigned short id_prev = 0, dropped_frames = 0;
  unsigned int frames_recd = 0;

	//Grab MAX_FRAMES frames and do nothing with the data for now.
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
        printf("fin del for");

		ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);

    if(!ReturnVal)
    {
      //Counting dropped frames through frame_id as redundant check. 
      if(((evtFrameRecv.frame_id) != id_prev+1) && (frame_count != 0)) //Ignore very first frame as id is unknown.
      {
        dropped_frames++;
        //printf("\ndfe!\n");
        //printf("%d %d %d\n", evtFrameRecv.frame_id, id_prev, frame_count);
      }
      else
        frames_recd++;
    }
    else
    {
      dropped_frames++;
      printf("\nEVT_CameraGetFrame Error = %8.8x!\n", ReturnVal);
    }

    //In GVSP there is no id 0 so when 16 bit id counter in camera is max then the next id is 1 so set prev id to 0 for math above.
    if(evtFrameRecv.frame_id == 65535)
      id_prev = 0;
    else
      id_prev = evtFrameRecv.frame_id;

    if(ReturnVal)
      break; //No requeue reqd

    ReturnVal = EVT_CameraQueueFrame(&camera, &evtFrameRecv); //Re-queue.

    if(ReturnVal) printf("EVT_CameraQueueFrame Error!\n");
    if(frame_count % 100 == 99) 
    {
      printf(".");
#ifndef _MSC_VER 
      fflush(stdout);
#endif
    }

    
    if(frame_count % 10000 == 9999) printf("\n");

    if(dropped_frames >= 100)
      break;

  }

  //End timer
#ifdef _MSC_VER 
  clock_t end=clock();
  double time_diff = (double(static_cast<unsigned __int64>(end)-begin))/CLOCKS_PER_SEC;
#else
  gettimeofday(&endTime, NULL);
  float time_diff = (endTime.tv_sec  - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000000.0;
#endif


	//Tell camera to stop streaming
  EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

  //Report stats
  printf("\n");
  printf("Images Captured: \t%d\n", frames_recd);
  printf("Dropped Frames: \t%d\n", dropped_frames);
  printf("Frame Rate Setting: \t%d\n", frame_rate);
  printf("Calculated Frame Rate: \t%f\n", frames_recd/time_diff);

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
