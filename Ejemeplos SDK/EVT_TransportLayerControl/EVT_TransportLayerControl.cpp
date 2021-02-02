/*****************************************************************************
 * EVT_TransportLayerControl.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_TransportLayerControl
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_TransportLayerControl example opens the device, sets packet size 
 *   to max, starts streaming, receives frames, then stops streaming, closes the device and ends.
 *   Thus it exercises functionality of the XML TransportLayerControl group.
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
#define MAX_FRAMES 50

#define MAX_CAMERAS 10

void configure_defaults(CEmergentCamera* camera);
char* next_token;

int main(int argc, char* argv[])
{
  CEmergentCamera camera;
  int ReturnVal = SUCCESS;
  unsigned int param_val_max, param_val_min, param_val_inc;
  unsigned int height_max, width_max;
  CEmergentFrame evtFrame;
  char filename[100];
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;

  printf("-----------------------------------------"); printf("\n");
  printf("TransportLayerControl : Example program  "); printf("\n");
  printf("-----------------------------------------"); printf("\n");

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

  //Get max and min allowable param values. Error checking omitted for clarity.
  EVT_CameraGetUInt32ParamMax(&camera, "GevSCPSPacketSize", &param_val_max);
  printf("PacketSize Max: \t%d\n", param_val_max);
	EVT_CameraGetUInt32ParamMin(&camera, "GevSCPSPacketSize", &param_val_min);
  printf("PacketSize Min: \t%d\n", param_val_min);
  EVT_CameraGetUInt32ParamInc(&camera, "GevSCPSPacketSize", &param_val_inc);
  printf("PacketSize Inc: \t%d\n", param_val_inc);

  //Set packet size to maximum supported. Error checking omitted for clarity.
	EVT_CameraSetUInt32Param(&camera, "GevSCPSPacketSize", param_val_max);
  printf("PacketSize Set: \t%d\n", param_val_max);

	//Prepare host side for streaming.
	ReturnVal = EVT_CameraOpenStream(&camera);
  if(ReturnVal != SUCCESS)
  {
    printf("CameraOpenStream: Error\n");
    return ReturnVal;
  }

  //Allocate buffer and queue up frame before entering grab loop.

  //Three params used for memory allocation. Worst case covers all models so no recompilation required.
  evtFrame.size_x = width_max;
  evtFrame.size_y = height_max;
  evtFrame.pixel_type = GVSP_PIX_MONO8;  //Covers color model using BayerGB8 also.
  EVT_AllocateFrameBuffer(&camera, &evtFrame, EVT_FRAME_BUFFER_ZERO_COPY);
  EVT_CameraQueueFrame(&camera, &evtFrame);

  printf("Grabbing Frames...\n");

	//Grab MAX_FRAMES frames and do nothing with the data for now.
  unsigned int frames_recd = 0;
	for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	{
	  //Tell camera to start streaming
	  ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
    if(ReturnVal != SUCCESS)
    {
      printf("CameraExecuteCommand: Error\n");
      return ReturnVal;
    }

		err = EVT_CameraGetFrame(&camera, &evtFrame, EVT_INFINITE);
    if(err) 
      printf("EVT_CameraGetFrame Error = %8.8x!\n", err);
    else
      frames_recd++;

	  //Tell camera to stop streaming
	  EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

#ifdef _MSC_VER
    sprintf_s(filename, "myimage_%d.tif", frame_count);
#else
    sprintf(filename, "myimage_%d.tif", frame_count);
#endif

    EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);

    printf(".");
#ifndef _MSC_VER 
    fflush(stdout);
#endif

    EVT_CameraQueueFrame(&camera, &evtFrame);  //Now re-queue.
	}

  printf("\nImages Captured: \t%d\n", frames_recd);

  //Release frame buffer
  EVT_ReleaseFrameBuffer(&camera, &evtFrame);

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
