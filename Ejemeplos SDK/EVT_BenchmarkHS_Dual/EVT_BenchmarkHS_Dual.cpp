/*****************************************************************************
 * EVT_BenchmarkHS_Dual.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_BenchmarkHS_Dual
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_BenchmarkHS_Dual example opens two like model cameras and configures
 *   them for highest speed and counts dropped frames and then calculates
 *   the effective frame rate. 0 dropped frames is expected.
 *   This example does not use multiple threads to test.
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
#include <time.h>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>

using namespace std;
using namespace Emergent;

#define SUCCESS 0

//#define XML_FILE_1   "C:\\xml\\Emergent_HS-2000-M_1_0.xml"
//#define XML_FILE_2   "C:\\xml\\Emergent_HS-2000-M_1_0.xml"

#define MAX_FRAMES 1000
#define FRAMES_BUFFERS 30

#define CAMERAS 2
#define MAX_CAMERAS 10

void configure_defaults(CEmergentCamera* camera);
char* next_token;

void camera_thread(void* arg);

int main(int argc, char *argv[])
{
  CEmergentCamera camera[CAMERAS];
  int ReturnVal = SUCCESS;
  unsigned int frame_rate_max, frame_rate_min, frame_rate_inc, frame_rate;
  unsigned int height_max, width_max;
  
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index, numCameras = 0, evt_index[CAMERAS], cam;

  printf("------------------------------------"); printf("\n");
  printf("BenchmarkHS_Dual : Example program  "); printf("\n");
  printf("------------------------------------"); printf("\n");

  //Find all cameras in system.
  unsigned int listcam_buf_size = MAX_CAMERAS;
  EVT_ListDevices(deviceInfo, &listcam_buf_size, &count);
  if(count<CAMERAS)
  {
    printf("Enumerate Cameras: \tNeed at least 2 EVT cameras to run example. Exiting program.\n");
    return 0;
  }

  //Find 2 EVT cameras
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
        evt_index[numCameras] = camera_index;
        numCameras++;
        if(numCameras == 1)
        {
            break; //Found 2 EVT camera so break. cam_index carries indexes for open.
        }
    }

    if(camera_index==(CAMERAS-1))
    {
      printf("2 EVT cameras not found. Exiting program\n");
      return 0;
    }
  }  

  for(cam=0;cam<2;cam++)
  {
    //Open the camera. Example usage. Camera found needs to match XML.
  #ifdef XML_FILE
    ReturnVal = EVT_CameraOpen(&camera, &deviceInfo[camera_index], XML_FILE);
  #else
    ReturnVal = EVT_CameraOpen(&camera[cam], &deviceInfo[evt_index[cam]]);      
  #endif
    if(ReturnVal != SUCCESS)
    {
      printf("Open Camera: \t\tError. Exiting program.\n");
      return ReturnVal;
    }
    else
    {
      printf("Open Camera: \t\tCamera Opened: %s\n", deviceInfo[evt_index[cam]].modelName);
    }
  }

  for(cam=0;cam<2;cam++)
  {
    //To avoid conflict with settings in other examples.
    configure_defaults(&camera[cam]);
  }

  for(cam=0;cam<2;cam++)
  {
    printf("Camera %d:\n", cam);
    //Get resolution.
    EVT_CameraGetUInt32ParamMax(&camera[cam], "Height", &height_max);
    EVT_CameraGetUInt32ParamMax(&camera[cam], "Width" , &width_max);
    printf("Resolution: \t\t%d x %d\n", width_max, height_max);

    //Frame rate
    EVT_CameraGetUInt32ParamMax(&camera[cam], "FrameRate", &frame_rate_max);
    printf("FrameRate Max: \t\t%d\n", frame_rate_max);
    EVT_CameraGetUInt32ParamMin(&camera[cam], "FrameRate", &frame_rate_min);
    printf("FrameRate Min: \t\t%d\n", frame_rate_min);
    EVT_CameraGetUInt32ParamInc(&camera[cam], "FrameRate", &frame_rate_inc);
    printf("FrameRate Inc: \t\t%d\n", frame_rate_inc);

    frame_rate = frame_rate_max;
    EVT_CameraSetUInt32Param(&camera[cam], "FrameRate", frame_rate);
    printf("FrameRate Set: \t\t%d\n", frame_rate);
  }

#ifdef _MSC_VER
  HANDLE camera_thread_handle[2];
  //Open one grabbing/processing thread for each camera.
  for(cam=0;cam<2;cam++)
  {
    camera_thread_handle[cam] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&camera_thread, (void*)&camera[cam], 0, NULL);
  }
  
  //Now wait for threads to complete signifying that all frames received.
  WaitForMultipleObjects(2, camera_thread_handle, TRUE, -1);
#else
  pthread_t camera_thread_handle[2];
  //Open one grabbing/processing thread for each camera.
  for(cam=0;cam<2;cam++)
  {
	if(pthread_create(&camera_thread_handle[cam], NULL, (void* (*)(void*))&camera_thread, (void*)&camera[cam]) != 0)
	{
		printf("Failed to create thread.\n");
		break;
	}
  }
  //Now wait for threads to complete signifying that all frames received.  
    for(cam=0;cam<2;cam++)
    {
        if(pthread_join(camera_thread_handle[cam], NULL) != 0)
	{
		printf("Failed in thread join.\n");
	}
    }
#endif

  for(cam=0;cam<2;cam++)
  {
    //To avoid conflict with settings in other examples.
    configure_defaults(&camera[cam]);
  }

  for(cam=0;cam<2;cam++)
  {
	  EVT_CameraClose(&camera[cam]);
    printf("\nClose Camera: \t\tCamera Closed");
  }

  return(0);
} 

void camera_thread(void* arg)
{
  int ReturnVal = SUCCESS;
  CEmergentFrame evtFrame[FRAMES_BUFFERS];
  CEmergentCamera* camera = (CEmergentCamera*)arg;

  printf("Thread started\n");

	//Prepare host side for streaming.
	ReturnVal = EVT_CameraOpenStream(camera);
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraOpenStream: Error\n");
    return;
  }

  //Allocate buffers and queue up frames before entering grab loop.
	for(int frame_count=0;frame_count<FRAMES_BUFFERS;frame_count++)
	{
    //Three params used for memory allocation. Worst case covers all models so no recompilation required.
    evtFrame[frame_count].size_x = 4096;
    evtFrame[frame_count].size_y = 3072;
    evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8;  //Covers color model using BayerGB8 also.
    ReturnVal = EVT_AllocateFrameBuffer(camera, &evtFrame[frame_count], EVT_FRAME_BUFFER_ZERO_COPY);
    if(ReturnVal) printf("EVT_AllocateFrameBuffer Error!\n");
    ReturnVal = EVT_CameraQueueFrame(camera, &evtFrame[frame_count]);
    if(ReturnVal) printf("EVT_CameraQueueFrame Error!\n");
	}

	//Tell camera to start streaming
	ReturnVal = EVT_CameraExecuteCommand(camera, "AcquisitionStart");
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraExecuteCommand: Error\n");
    return;
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

  for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
		ReturnVal = EVT_CameraGetFrame(camera, &evtFrame[frame_count % FRAMES_BUFFERS], EVT_INFINITE);
    if(!ReturnVal)
    {
      //Counting dropped frames through frame_id as redundant check. 
      if(((evtFrame[frame_count % FRAMES_BUFFERS].frame_id) != id_prev+1) && (frame_count != 0)) //Ignore very first frame as id is unknown.
      {
        dropped_frames++;
        printf("\ndfe!\n");
        printf("%d %d %d\n", evtFrame[frame_count % FRAMES_BUFFERS].frame_id, id_prev, frame_count);
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
    if(evtFrame[frame_count % FRAMES_BUFFERS].frame_id == 65535)
      id_prev = 0;
    else
      id_prev = evtFrame[frame_count % FRAMES_BUFFERS].frame_id;

    if(ReturnVal)
      break; //No requeue reqd

    ReturnVal = EVT_CameraQueueFrame(camera, &evtFrame[frame_count % FRAMES_BUFFERS]); //Re-queue.

    if(ReturnVal) printf("EVT_CameraQueueFrame Error!\n");
    if(frame_count % 100 == 99) 
    {
      printf(".");
#ifndef _MSC_VER 
      fflush(stdout);
#endif
    }
//    if(frame_count % 10000 == 9999) printf("\n");

  }

  //End timer
#ifdef _MSC_VER 
  clock_t end=clock();
  double time_diff = (double(end-begin))/CLOCKS_PER_SEC;
#else
  gettimeofday(&endTime, NULL);
  float time_diff = (endTime.tv_sec  - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000000.0;
#endif

  //Tell camera to stop streaming
  EVT_CameraExecuteCommand(camera, "AcquisitionStop");

  //Release frame buffers
	for(int frame_count=0;frame_count<FRAMES_BUFFERS;frame_count++)
	{
		EVT_ReleaseFrameBuffer(camera, &evtFrame[frame_count]);
	}

	//Host side tear down for stream.
	EVT_CameraCloseStream(camera);

  //Report stats
  printf("\n");
  printf("Images Captured: \t%d\n", frames_recd);
  printf("Dropped Frames: \t%d\n", dropped_frames);
  printf("Calculated Frame Rate: \t%f\n", frames_recd/time_diff);

  printf("\nThread finished\n");
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


