/*****************************************************************************
 * EVT_DeviceInformation.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_DeviceInformation
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_DeviceInformation example opens the device and queries
 *   and displays the information from the XML DeviceInformation group
 *   and then closes the device.
 *   Thus it exercises functionality of the XML DeviceInformation group.
 ****************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <EmergentCameraAPIs.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>


using namespace std;
using namespace Emergent;

#define SUCCESS 0
//#define XML_FILE   "C:\\xml\\Emergent_HS-2000-M_1_0.xml"

#define MAX_CAMERAS 10

void configure_defaults(CEmergentCamera* camera);
char* next_token;

int main(int argc, char* argv[])
{
  CEmergentCamera camera;
  const unsigned long StringSize = 256;
  unsigned long StringSizeReturn = 0;
  char StringBuffer[StringSize];
  int ReturnVal = SUCCESS;
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;

  printf("-------------------------------------"); printf("\n");
  printf("DeviceInformation : Example program  "); printf("\n");
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

	//Get Device information.
	ReturnVal = EVT_CameraGetStringParam(&camera, "DeviceVendorName", StringBuffer, StringSize, &StringSizeReturn);
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraGetStringParam: Error\n");
    return ReturnVal;
  }
  printf("DeviceVendorName: \t%s\n", StringBuffer);

	ReturnVal = EVT_CameraGetStringParam(&camera, "DeviceModelName", StringBuffer, StringSize, &StringSizeReturn);
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraGetStringParam: Error\n");
    return ReturnVal;
  }
  printf("DeviceModelName: \t%s\n", StringBuffer);

	ReturnVal = EVT_CameraGetStringParam(&camera, "DeviceVersion", StringBuffer, StringSize, &StringSizeReturn);
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraGetStringParam: Error\n");
    return ReturnVal;
  }
  printf("DeviceVersion: \t\t%s\n", StringBuffer);

	ReturnVal = EVT_CameraGetStringParam(&camera, "DeviceSerialNumber", StringBuffer, StringSize, &StringSizeReturn);
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraGetStringParam: Error\n");
    return ReturnVal;
  }
  printf("DeviceSerialNumber: \t%s\n", StringBuffer);

	ReturnVal = EVT_CameraGetStringParam(&camera, "DeviceUserName", StringBuffer, StringSize, &StringSizeReturn);
  if(ReturnVal != SUCCESS)
  {
    printf("EVT_CameraGetStringParam: Error\n");
    return ReturnVal;
  }
  printf("DeviceFirmwareVersion: \t%s\n", StringBuffer);

  //To avoid conflict with settings in other examples.
  configure_defaults(&camera);

	//Close the camera
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

  EVT_CameraSetUInt32Param(camera,    "FrameRate", 60);

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
