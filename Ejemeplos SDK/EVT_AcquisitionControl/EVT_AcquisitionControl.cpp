/*****************************************************************************
 * EVT_AcquisitionControl.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_AcquisitionControl
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_AcquisitionControl example opens the device, sets exposure, tests  
 *   software multiframe trigger, tests multiframe software trigger read of 
 *   buffer, then closes the device and ends. 
 *   Thus it exercises functionality of the XML AcquisitionControl group.
 *   Other elements exercised elsewhere.
 ****************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentFrameSave.h>

using namespace std;
using namespace Emergent;

#define SUCCESS 0
//#define XML_FILE   "C:\\xml\\Emergent_HS-2000-M_1_0.xml"
#define MAX_FRAMES 30

#define MAX_CAMERAS 1

#define EXPOSURE_VAL 1000 //us

void configure_defaults(CEmergentCamera* camera);
char* next_token;

int main(int argc, char* argv[])
{
  CEmergentCamera camera;
  int ReturnVal = SUCCESS;
  unsigned int param_val_max, param_val_min, param_val_inc, height_max, width_max;
  CEmergentFrame evtFrame[MAX_FRAMES];
  char filename[100];
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;

  printf("-------------------------------------"); printf("\n");
  printf("AcquisitionControl : Example program "); printf("\n");
  printf("-------------------------------------"); printf("\n");

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

  //Set exposure. Error check omitted for clarity.
  EVT_CameraGetUInt32ParamMax(&camera, "Exposure", &param_val_max);
  printf("Exposure Max: \t\t%d\n", param_val_max);
  EVT_CameraGetUInt32ParamMin(&camera, "Exposure", &param_val_min);
  printf("Exposure Min: \t\t%d\n", param_val_min);
  EVT_CameraGetUInt32ParamInc(&camera, "Exposure", &param_val_inc);
  printf("Exposure Inc: \t\t%d\n", param_val_inc);
  if(EXPOSURE_VAL >= param_val_min && EXPOSURE_VAL <= param_val_max)
  {
    EVT_CameraSetUInt32Param(&camera, "Exposure", EXPOSURE_VAL);
    printf("Exposure Set: \t\t%d\n", EXPOSURE_VAL);
  }

  //Setup multiframe software trigger for MAX_FRAMES frame. Error check omitted for clarity.
  printf("Trigger Mode: \t\tMulti Frame, SoftwareTrigger, %d Frames\n", MAX_FRAMES);
  EVT_CameraSetEnumParam(&camera,   "AcquisitionMode",        "MultiFrame");
  EVT_CameraSetUInt32Param(&camera, "AcquisitionFrameCount",  MAX_FRAMES);
  EVT_CameraSetEnumParam(&camera,   "TriggerSelector",        "FrameStart");
  EVT_CameraSetEnumParam(&camera,   "TriggerMode",            "On");
  EVT_CameraSetEnumParam(&camera,   "TriggerSource",          "Software");
  EVT_CameraSetEnumParam(&camera,   "BufferMode",             "Off");
  EVT_CameraSetUInt32Param(&camera, "BufferNum",              0);

	//Prepare host side for streaming.
	EVT_CameraOpenStream(&camera);

	//Tell camera to start streaming(in SW trig mode need following TriggerSoftware to get frames)
	EVT_CameraExecuteCommand(&camera, "AcquisitionStart");

  //Allocate buffers and queue up frames before entering grab loop.
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
    //Three params used for memory allocation. Worst case covers all models so no recompilation required.
    evtFrame[frame_count].size_x = width_max;
    evtFrame[frame_count].size_y = height_max;
    evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8; //Covers color model using BayerGB8 also.
    err = EVT_AllocateFrameBuffer(&camera, &evtFrame[frame_count], EVT_FRAME_BUFFER_ZERO_COPY);
    if(err) printf("EVT_AllocateFrameBuffer Error!\n");
    err = EVT_CameraQueueFrame(&camera, &evtFrame[frame_count]);
    if(err) printf("EVT_CameraQueueFrame Error!\n");
	}

  //Now, do multiframe software trigger grab. Error check omitted for clarity.
  EVT_CameraExecuteCommand(&camera, "TriggerSoftware");

  printf("Grabbing Frames...\n");

	//Now read frames back.
  unsigned int frames_recd = 0;
  for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
  {
		err = EVT_CameraGetFrame(&camera, &evtFrame[frame_count], EVT_INFINITE);
   if(err) 
     printf("EVT_CameraGetFrame Error!\n");
    else
      frames_recd++;

    printf(".");
#ifndef _MSC_VER 
    fflush(stdout);
#endif
	}

	//Tell camera to disarm streaming.
	EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

  //Drop into files offline.
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{

#ifdef _MSC_VER
    sprintf_s(filename, "myimage_%d.tif", frame_count);
#else
    sprintf(filename, "myimage_%d.tif", frame_count);
#endif

    EVT_FrameSave(&evtFrame[frame_count], filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
	}
  
  printf("\nImages Captured: \t%d\n", frames_recd);

  //Tell camera to re-arm streaming. Required in multiframe mode.
  EVT_CameraExecuteCommand(&camera, "AcquisitionStart");

  //Now setup to read back same MAX_FRAMES except this time from the camera buffer memory and one frame at a time.
  printf("Trigger Mode: \t\tMulti Frame, SoftwareTrigger, %d Frames, Buffer Mode\n", MAX_FRAMES);
  EVT_CameraSetEnumParam(&camera, "BufferMode", "Buffer");
  EVT_CameraSetUInt32Param(&camera, "AcquisitionFrameCount", 1);
  
  //Queue up MAX_FRAMES frames again.
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
    err = EVT_CameraQueueFrame(&camera, &evtFrame[frame_count]);
    if(err) printf("EVT_CameraQueueFrame Error!\n");
	}

  printf("Grabbing Frames...\n");

  //Now read frames back.
  frames_recd = 0;
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
    EVT_CameraSetUInt32Param(&camera, "BufferNum", frame_count);
    EVT_CameraExecuteCommand(&camera, "TriggerSoftware");

		err = EVT_CameraGetFrame(&camera, &evtFrame[frame_count], EVT_INFINITE);
    if(err) 
      printf("EVT_CameraGetFrame Error!\n");
    else
      frames_recd++;
    
    printf(".");
#ifndef _MSC_VER 
    fflush(stdout);
#endif

    //Disarm and re-arm for multiframe triggering.
	  EVT_CameraExecuteCommand(&camera, "AcquisitionStop");
    EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
	}

  //Tell camera to disarm streaming.
	EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

  //Drop into files offline.
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
    
#ifdef _MSC_VER
    sprintf_s(filename, "myimage_buff_%d.tif", frame_count);
#else
    sprintf(filename, "myimage_buff_%d.tif", frame_count);
#endif

    EVT_FrameSave(&evtFrame[frame_count], filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
	}

  printf("\nImages Captured: \t%d\n", frames_recd);

  //Release frame buffers
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
		EVT_ReleaseFrameBuffer(&camera, &evtFrame[frame_count]);
	}
    
  //Unconfigure acquisition.
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
