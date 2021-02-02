/*****************************************************************************
 * EVT_PTP.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_PTP
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************

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
  #include <unistd.h>
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
#define MAX_FRAMES 1001
#define FRAMES_BUFFERS 30

#define MAX_CAMERAS 10


void configure_defaults(CEmergentCamera* camera);
char* next_token;
int cpuToUse;

int main(int argc, char *argv[])
{
  CEmergentCamera camera;
  int ReturnVal = SUCCESS, ptp_offset, ptp_offset_sum=0, ptp_offset_prev=0;
  unsigned int frame_rate_max, frame_rate_min, frame_rate_inc, frame_rate, ptp_time_plus_delta_to_start_uint;
  unsigned int height_max, width_max, ptp_time_low, ptp_time_high, ptp_time_plus_delta_to_start_low, ptp_time_plus_delta_to_start_high;
  unsigned long long ptp_time_delta_sum = 0, ptp_time_delta, ptp_time, ptp_time_prev, ptp_time_plus_delta_to_start, ptp_time_countdown;
  unsigned long long frame_ts, frame_ts_prev, frame_ts_delta, frame_ts_delta_sum = 0;
  CEmergentFrame evtFrame[FRAMES_BUFFERS], evtFrameRecv;
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;
  int cameras_found = 0;
  char ptp_status[100];
  unsigned long ptp_status_sz_ret;
  const unsigned long StringSize = 256;
  unsigned long StringSizeReturn = 0;
  char StringBuffer[StringSize];

  printf("-------------------------------"); printf("\n");
  printf("EVT_PTP : Example program      "); printf("\n");
  printf("-------------------------------"); printf("\n");

 
  //If argc = 4 then passing time to start acquisition, if argc = 3 then just reading current time.
  if(argc == 4)
  {
    ptp_time_plus_delta_to_start_uint = atoi(argv[3]);
    ptp_time_plus_delta_to_start = ((unsigned long long)ptp_time_plus_delta_to_start_uint) * 1000000000;
  }

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
    char* EVT_models[] = {"HS", "HT", "HR", "HB"};
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
    printf("Open Camera index %d: \t\tCamera Opened\n\n", camera_index);
  }

  EVT_CameraGetStringParam(&camera, "DeviceSerialNumber", StringBuffer, StringSize, &StringSizeReturn);
  printf("DeviceSerialNumber: \t%s\n", StringBuffer);


  //This code only executed if time to start triggering not supplied on command line
  //and it simply gets the current PTP time from the camera.
  //Ptp server should be serving time to the camera before this is run.
  if (argc != 4)
  {
    EVT_CameraSetEnumParam(&camera, "PtpMode", "OneStep");

    //Just wait a bit for camera to get PTP time. 
#ifdef _MSC_VER
    Sleep(5000);
#else
    sleep(5);
#endif

    //Get and print PTP stutus
    EVT_CameraGetEnumParam(&camera, "PtpStatus", ptp_status, sizeof(ptp_status), &ptp_status_sz_ret);
    printf("PTP Status: %s\n", ptp_status);

    //Get and print current time.    
    EVT_CameraExecuteCommand(&camera, "GevTimestampControlLatch");
    EVT_CameraGetUInt32Param(&camera, "GevTimestampValueHigh", &ptp_time_high);
    EVT_CameraGetUInt32Param(&camera, "GevTimestampValueLow", &ptp_time_low);
    ptp_time = (((unsigned long long)(ptp_time_high)) << 32) | ((unsigned long long)(ptp_time_low));
    printf("PTP Current Time(s): %llu\n", ptp_time / 1000000000);

    //Close out and exit.
    EVT_CameraSetEnumParam(&camera, "PtpMode", "Off");
    EVT_CameraClose(&camera);
    printf("\nClose Camera: \t\tCamera Closed\n");
    return 0;
  }

  //To avoid conflict with settings in other examples.
  configure_defaults(&camera);

  //PTP triggering configuration settings
  EVT_CameraSetEnumParam(&camera, "TriggerSource", "Software");
  EVT_CameraSetEnumParam(&camera, "AcquisitionMode", "MultiFrame");
  EVT_CameraSetUInt32Param(&camera, "AcquisitionFrameCount", 1);
  EVT_CameraSetEnumParam(&camera, "TriggerMode", "On");
  EVT_CameraSetEnumParam(&camera, "PtpMode", "OneStep");

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

  frame_rate = frame_rate_max-5;
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
      else if(strcmp(argv[2], "BGR8Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_BGR8;
      }
      else if(strcmp(argv[2], "BGR10Packed") == 0)
      {
        evtFrame[frame_count].pixel_type = GVSP_PIX_BGR10;
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


//////////////////////////////PTP time to start in future////////////////////////////////////

  //Ptp status and offset check.
  EVT_CameraGetEnumParam(&camera, "PtpStatus", ptp_status, sizeof(ptp_status), &ptp_status_sz_ret);
  printf("PTP Status: %s\n", ptp_status);

  //Show raw offsets.
  for (unsigned int i = 0; i < 5;)
  {
    EVT_CameraGetInt32Param(&camera, "PtpOffset", &ptp_offset);
    if (ptp_offset != ptp_offset_prev)
    {
      ptp_offset_sum += ptp_offset;
      i++;
      printf("Offset %d: %d\n", i, ptp_offset);
    }

    ptp_offset_prev = ptp_offset;
  }

  //Offset average.
  printf("Offset Average: %d\n", ptp_offset_sum / 5);

  ptp_time_plus_delta_to_start_low  = (unsigned int)(ptp_time_plus_delta_to_start & 0xFFFFFFFF);
  ptp_time_plus_delta_to_start_high = (unsigned int)(ptp_time_plus_delta_to_start >> 32);
  EVT_CameraSetUInt32Param(&camera, "PtpAcquisitionGateTimeHigh", ptp_time_plus_delta_to_start_high);
  EVT_CameraSetUInt32Param(&camera, "PtpAcquisitionGateTimeLow", ptp_time_plus_delta_to_start_low);

  printf("PTP Gate time(ns): %llu\n", ptp_time_plus_delta_to_start);

//////////////////////////////PTP time to start in future////////////////////////////////////

  //Tell camera to start streaming
  ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraExecuteCommand: Error\n");
    return ReturnVal;
  }

  printf("Grabbing Frames after countdown...\n");

  ptp_time_countdown = 0;

  //Countdown code
  do {
    EVT_CameraExecuteCommand(&camera, "GevTimestampControlLatch");
    EVT_CameraGetUInt32Param(&camera, "GevTimestampValueHigh", &ptp_time_high);
    EVT_CameraGetUInt32Param(&camera, "GevTimestampValueLow", &ptp_time_low);

    ptp_time = (((unsigned long long)(ptp_time_high)) << 32) | ((unsigned long long)(ptp_time_low));

    if (ptp_time > ptp_time_countdown)
    {
      printf("%llu\n", (ptp_time_plus_delta_to_start - ptp_time) / 1000000000);
      ptp_time_countdown = ptp_time + 1000000000; //1s
    }

  } while (ptp_time <= ptp_time_plus_delta_to_start);
  //Countdown done.


  printf("\n");
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
    ReturnVal = EVT_CameraGetFrame(&camera, &evtFrameRecv, EVT_INFINITE);

//////////////////////////////PTP timestamp checking////////////////////////////////////
    EVT_CameraExecuteCommand(&camera, "GevTimestampControlLatch");
    EVT_CameraGetUInt32Param(&camera, "GevTimestampValueHigh", &ptp_time_high);
    EVT_CameraGetUInt32Param(&camera, "GevTimestampValueLow", &ptp_time_low);

    ptp_time = (((unsigned long long)(ptp_time_high)) << 32) | ((unsigned long long)(ptp_time_low));
    frame_ts = evtFrameRecv.timestamp;

    if (frame_count != 0)
    {
      ptp_time_delta = ptp_time - ptp_time_prev;
      ptp_time_delta_sum += ptp_time_delta;

      frame_ts_delta = frame_ts - frame_ts_prev;
      frame_ts_delta_sum += frame_ts_delta;

    }

    ptp_time_prev = ptp_time;
    frame_ts_prev = frame_ts;
//////////////////////////////PTP timestamp checking////////////////////////////////////

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

	if(frame_count % 100 == 99) printf(".\n");

    if(dropped_frames >= 100)
      break;
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
  EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

  //Report stats
  printf("\n");
  printf("Images Captured: \t%d\n", frames_recd);
  printf("Dropped Frames: \t%d\n", dropped_frames);
  printf("Frame Rate Setting: \t%d\n", frame_rate);
  printf("Frame Rate Meas1: \t%f\n", frames_recd/time_diff);
  printf("Frame Rate Meas2: \t%f\n", ((float)(1000000000) * (float)(MAX_FRAMES - 1)) / ((float)(ptp_time_delta_sum)));
  printf("Frame Rate Meas3: \t%f\n", ((float)(1000000000) * (float)(MAX_FRAMES - 1)) / ((float)(frame_ts_delta_sum)));
  

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
  EVT_CameraSetUInt32Param(camera,    "Exposure", 1000);

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
