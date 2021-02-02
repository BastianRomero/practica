/*****************************************************************************
 * EVT_AnalogControl.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_AnalogControl
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_AnalogControl example opens the device, sets gain, offset, LUT, 
 *   and auto gain, starts streaming, receives frames, stops streaming, closes 
 *   the device and ends.
 *   Thus it exercises functionality of the XML AnalogControl group.
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
#define MAX_FRAMES 500

#define MAX_CAMERAS 10

#define TRUE  1
#define FALSE 0

#define GAIN_VAL      500
#define OFFSET_VAL    0
#define AG_SET_VAL    512
#define AG_I_VAL      16

void configure_defaults(CEmergentCamera* camera);
char* next_token;

int main(int argc, char* argv[])
{
  CEmergentCamera camera;
  struct EvtParamAttribute attribute;
  int ReturnVal = SUCCESS;
  unsigned int param_val_max, param_val_min, param_val_inc;
  unsigned int height_max, width_max;
  CEmergentFrame evtFrame;
  char filename[100];
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;

  printf("-------------------------------------"); printf("\n");
  printf("AnalogControl : Example program      "); printf("\n");
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

  //Check if gain supported and then set to value as in #define above if within range.
	ReturnVal = EVT_CameraGetParamAttr(&camera, "Gain", &attribute);
  if (attribute.dataType == EDataTypeUnsupported)
  {
    printf("CameraGetParamAttr: Error\n");
    return ReturnVal;
  }
  else
  {
    EVT_CameraGetUInt32ParamMax(&camera, "Gain", &param_val_max);
    printf("Gain Max: \t\t%d\n", param_val_max);
    EVT_CameraGetUInt32ParamMin(&camera, "Gain", &param_val_min);
    printf("Gain Min: \t\t%d\n", param_val_min);
    EVT_CameraGetUInt32ParamInc(&camera, "Gain", &param_val_inc);
    printf("Gain Inc: \t\t%d\n", param_val_inc);
    if(GAIN_VAL >= param_val_min && GAIN_VAL <= param_val_max)
    {
      EVT_CameraSetUInt32Param(&camera, "Gain", GAIN_VAL);
      printf("Gain Set: \t\t%d\n", GAIN_VAL);
    }
  }

  //Check if offset supported and then set to value as in #define above if within range.
	ReturnVal = EVT_CameraGetParamAttr(&camera, "Offset", &attribute);
  if (attribute.dataType == EDataTypeUnsupported)
  {
    printf("CameraGetParamAttr: Error\n");
    return ReturnVal;
  }
  else
  {
    EVT_CameraGetUInt32ParamMax(&camera, "Offset", &param_val_max);
    printf("Offset Max: \t\t%d\n", param_val_max);
    EVT_CameraGetUInt32ParamMin(&camera, "Offset", &param_val_min);
    printf("Offset Min: \t\t%d\n", param_val_min);
    EVT_CameraGetUInt32ParamInc(&camera, "Offset", &param_val_inc);
    printf("Offset Inc: \t\t%d\n", param_val_inc);
    if(OFFSET_VAL >= param_val_min && OFFSET_VAL <= param_val_max)
    {
      EVT_CameraSetUInt32Param(&camera, "Offset", OFFSET_VAL);
      printf("Offset Set: \t\t%d\n", OFFSET_VAL);
    }
  }

  //Check if LUT supported and then write a linear table.
	ReturnVal = EVT_CameraGetParamAttr(&camera, "LUTEnable", &attribute);
  if (attribute.dataType == EDataTypeUnsupported)
  {
    printf("CameraGetParamAttr: Error\n");
    return ReturnVal;
  }
  else
  {
    unsigned int lut_val_max, lut_val_min, lut_val_inc;
    unsigned int lut_addr_max, lut_addr_min, lut_addr_inc;
    EVT_CameraGetUInt32ParamMax(&camera, "LUTValue", &lut_val_max);
    printf("LUT Value Max: \t\t%d\n", lut_val_max);
    EVT_CameraGetUInt32ParamMin(&camera, "LUTValue", &lut_val_min);
    printf("LUT Value Min: \t\t%d\n", lut_val_min);
    EVT_CameraGetUInt32ParamInc(&camera, "LUTValue", &lut_val_inc);
    printf("LUT Value Inc: \t\t%d\n", lut_val_inc);
    EVT_CameraGetUInt32ParamMax(&camera, "LUTIndex", &lut_addr_max);
    printf("LUT Index Max: \t\t%d\n", lut_addr_max);
    EVT_CameraGetUInt32ParamMin(&camera, "LUTIndex", &lut_addr_min);
    printf("LUT Index Min: \t\t%d\n", lut_addr_min);
    EVT_CameraGetUInt32ParamInc(&camera, "LUTIndex", &lut_addr_inc);
    printf("LUT Index Inc: \t\t%d\n", lut_addr_inc);


    for(unsigned int lut_addr=lut_addr_min;lut_addr<=lut_addr_max;lut_addr++)
    {
      EVT_CameraSetUInt32Param(&camera, "LUTIndex", lut_addr); //Address we are setting value for...
      EVT_CameraSetUInt32Param(&camera, "LUTValue", lut_addr); //and value to set(just val=addr).
    }

    EVT_CameraSetBoolParam(&camera, "LUTEnable", TRUE); //Enable LUT we just programmed.

    printf("LUT table: \t\tWrote linear table. Enabled.\n");
  }

  //Check if Auto gain supported and then simply disable it if enabled but still set values for illustration purposes.
	ReturnVal = EVT_CameraGetParamAttr(&camera, "AutoGain", &attribute);
  if (attribute.dataType == EDataTypeUnsupported)
  {
    printf("CameraGetParamAttr: Error\n");
    return ReturnVal;
  }
  else
  {
    unsigned int ag_set_val_max, ag_set_val_min, ag_set_val_inc;
    unsigned int ag_I_val_max, ag_I_val_min, ag_I_val_inc;
    
    EVT_CameraGetUInt32ParamMax(&camera, "AutoGainSet", &ag_set_val_max);
    printf("AG Set Point Max: \t%d\n", ag_set_val_max);
    EVT_CameraGetUInt32ParamMin(&camera, "AutoGainSet", &ag_set_val_min);
    printf("AG Set Point Min: \t%d\n", ag_set_val_min);
    EVT_CameraGetUInt32ParamInc(&camera, "AutoGainSet", &ag_set_val_inc);
    printf("AG Set Point Inc: \t%d\n", ag_set_val_inc);
    EVT_CameraGetUInt32ParamMax(&camera, "AutoGainIGain", &ag_I_val_max);
    printf("AG Rate Max: \t\t%d\n", ag_I_val_max);
    EVT_CameraGetUInt32ParamMin(&camera, "AutoGainIGain", &ag_I_val_min);
    printf("AG Rate Min: \t\t%d\n", ag_I_val_min);
    EVT_CameraGetUInt32ParamInc(&camera, "AutoGainIGain", &ag_I_val_inc);
    printf("AG Rate Inc: \t\t%d\n", ag_I_val_inc);

    if(AG_SET_VAL >= ag_set_val_min && AG_SET_VAL <= ag_set_val_max)
    {
      EVT_CameraSetUInt32Param(&camera, "AutoGainSet", AG_SET_VAL);
      printf("AG Set Point: \t\t%d\n", AG_SET_VAL);
    }

    if(AG_I_VAL >= ag_I_val_min && AG_I_VAL <= ag_I_val_max)
    {
      EVT_CameraSetUInt32Param(&camera, "AutoGainIGain", AG_I_VAL);
      printf("AG Rate Set: \t\t%d\n", AG_I_VAL);
    }

    EVT_CameraSetBoolParam(&camera, "AutoGain", TRUE);
    printf("AutoGain Set: \t\tEnabled.\n");
  }

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
  evtFrame.pixel_type = GVSP_PIX_MONO8; //Covers color model using BayerGB8 also.
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
      printf("EVT_CameraGetFrame Error!\n");
    else
      frames_recd++;

	  //Tell camera to stop streaming
	  EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

    sprintf_s(filename, "D:/images/myimage_%d.tif", frame_count);

    EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); 

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
