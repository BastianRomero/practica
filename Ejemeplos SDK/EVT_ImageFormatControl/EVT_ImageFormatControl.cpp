/*****************************************************************************
 * EVT_ImageFormatControl.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_ImageFormatControl
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_ImageFormatControl example opens the device, sets ROI, starts streaming,
 *   receives frames and sets different pixel mode in between, stops streaming, 
 *   closes the device and ends.
 *   Thus it exercises functionality of the XML ImageFormatControl group.
 ****************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentFrameSave.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;
using namespace Emergent;

#define SUCCESS 0
//#define XML_FILE   "C:\\xml\\Emergent_HS-2000-M_1_0.xml"

#define MAX_CAMERAS 10

#define WIDTH_VAL      1024 //INC is 16 typ
#define HEIGHT_VAL     1044 //INC is 2 typ
#define OFFSET_X_VAL   256  //INC is 16 typ
#define OFFSET_Y_VAL   256  //INC is 2 typ

#define FRAMERATE_VAL  15

void configure_defaults(CEmergentCamera* camera);
char* next_token;

//#define RAW_FILE_SAVE
#define TIF_FILE_SAVE
//#define BMP_FILE_SAVE

int main(int argc, char* argv[])
{


  CEmergentCamera camera;
  int ReturnVal = SUCCESS;

  const unsigned long enumBufferSize = 1000;
  unsigned long enumBufferSizeReturn = 0;
  char enumBuffer[enumBufferSize];

  unsigned int width_max, width_min, width_inc;
  unsigned int height_max, height_min, height_inc;
  unsigned int offsetx_max, offsetx_min, offsetx_inc;
  unsigned int offsety_max, offsety_min, offsety_inc;
  CEmergentFrame evtFrame, evtFrameConvert;
  unsigned int enum_count=0;
  char filename[100];
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;

  printf("--------------------------------------"); printf("\n");
  printf("ImageFormatControl : Example program  "); printf("\n");
  printf("--------------------------------------"); printf("\n");

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

  //Set ROI width
  EVT_CameraGetUInt32ParamMax(&camera, "Width",   &width_max);
  printf("Width Max: \t\t%d\n", width_max);
  EVT_CameraGetUInt32ParamMin(&camera, "Width",   &width_min);
  printf("Width Min: \t\t%d\n", width_min);
  EVT_CameraGetUInt32ParamInc(&camera, "Width",   &width_inc);
  printf("Width Inc: \t\t%d\n", width_inc);

  if(WIDTH_VAL >= width_min && WIDTH_VAL <= width_max)
  {
    EVT_CameraSetUInt32Param(&camera, "Width", WIDTH_VAL);
    printf("Width Set: \t\t%d\n", WIDTH_VAL);
  }

  //Set ROI OffsetX. Now that Width changed we need to check new OffsetX limits
  EVT_CameraGetUInt32ParamMax(&camera, "OffsetX",   &offsetx_max);
  printf("OffsetX Max: \t\t%d\n", offsetx_max);
  EVT_CameraGetUInt32ParamMin(&camera, "OffsetX",   &offsetx_min);
  printf("OffsetX Min: \t\t%d\n", offsetx_min);
  EVT_CameraGetUInt32ParamInc(&camera, "OffsetX",   &offsetx_inc);
  printf("OffsetX Inc: \t\t%d\n", offsetx_inc);

  if(OFFSET_X_VAL >= offsetx_min && OFFSET_X_VAL <= offsetx_max)
  {
    EVT_CameraSetUInt32Param(&camera, "OffsetX", OFFSET_X_VAL);
    printf("OffsetX Set: \t\t%d\n", OFFSET_X_VAL);
  }

  //Set ROI height
  EVT_CameraGetUInt32ParamMax(&camera, "Height",   &height_max);
  printf("Height Max: \t\t%d\n", height_max);
  EVT_CameraGetUInt32ParamMin(&camera, "Height",   &height_min);
  printf("Height Min: \t\t%d\n", height_min);
  EVT_CameraGetUInt32ParamInc(&camera, "Height",   &height_inc);
  printf("Height Inc: \t\t%d\n", height_inc);

  if(HEIGHT_VAL >= height_min && HEIGHT_VAL <= height_max)
  {
    EVT_CameraSetUInt32Param(&camera, "Height", HEIGHT_VAL);
    printf("Height Set: \t\t%d\n", HEIGHT_VAL);
  }

  //Set ROI OffsetY. Now that height changed we need to check new OffsetY limits
  EVT_CameraGetUInt32ParamMax(&camera, "OffsetY",   &offsety_max);
  printf("OffsetY Max: \t\t%d\n", offsety_max);
  EVT_CameraGetUInt32ParamMin(&camera, "OffsetY",   &offsety_min);
  printf("OffsetY Min: \t\t%d\n", offsety_min);
  EVT_CameraGetUInt32ParamInc(&camera, "OffsetY",   &offsety_inc);
  printf("OffsetY Inc: \t\t%d\n", offsety_inc);

  if(OFFSET_Y_VAL >= offsety_min && OFFSET_Y_VAL <= offsety_max)
  {
    EVT_CameraSetUInt32Param(&camera, "OffsetY", OFFSET_Y_VAL);
    printf("OffsetY Set: \t\t%d\n", OFFSET_Y_VAL);
  }

  //Set frame rate to something smaller so that don't need to change for all pixel modes
  //as there is a dependency.
  EVT_CameraSetUInt32Param(&camera, "FrameRate", FRAMERATE_VAL);
  printf("Frame Rate Set: \t%d\n", FRAMERATE_VAL);

	//Prepare host side for streaming.
	ReturnVal = EVT_CameraOpenStream(&camera);
  if(ReturnVal != SUCCESS)
  {
    printf("CameraOpenStream: Error\n");
    return ReturnVal;
  }

  //Get all possible PixelFormats into comma seperated enumBuffer string array.
	EVT_CameraGetEnumParamRange(&camera, "PixelFormat", enumBuffer, enumBufferSize, &enumBufferSizeReturn);

  //Allocate buffer and queue up frame before entering grab loop.

  //Three params used for memory allocation. Worst case covers all models so no recompilation required.
  evtFrame.size_x = WIDTH_VAL;
  evtFrame.size_y = HEIGHT_VAL;
  evtFrame.pixel_type = GVSP_PIX_RGB10; 
  EVT_AllocateFrameBuffer(&camera, &evtFrame, EVT_FRAME_BUFFER_ZERO_COPY);

  //Allocate buffer for converted frame.

  //Five params used for memory allocation for converted frames. Worst case covers all models so no recompilation required.
  evtFrameConvert.size_x = WIDTH_VAL;
  evtFrameConvert.size_y = HEIGHT_VAL;
  evtFrameConvert.pixel_type = GVSP_PIX_RGB10;
  evtFrameConvert.convertColor = EVT_COLOR_CONVERT_BILINEAR;
  evtFrameConvert.convertBitDepth = EVT_CONVERT_8BIT;
  EVT_AllocateFrameBuffer(&camera, &evtFrameConvert, EVT_FRAME_BUFFER_DEFAULT);


  printf("Grabbing Frames(one for each pixel format)...\n");

  //Grab images for each of the supported PixelFormats.
  unsigned int frames_recd = 0;
  char* enumMember = strtok_s(enumBuffer, ",", &next_token);
  EVT_CameraSetEnumParam(&camera, "PixelFormat", enumMember);
  printf("PixelFormat: \t\t%d %s\n", enum_count, enumMember);
  while (enumMember != NULL) 
  { 
    //Queue frame.
    err = EVT_CameraQueueFrame(&camera, &evtFrame);
    if(err) 
      printf("EVT_CameraQueueFrame Error = %8.8x!\n", err);

	  //Tell camera to start streaming
	  ReturnVal = EVT_CameraExecuteCommand(&camera, "AcquisitionStart");
    if(ReturnVal != SUCCESS)
    {
      printf("CameraExecuteCommand: Error\n");
      return ReturnVal;
    }

    //Receive frame.
		err = EVT_CameraGetFrame(&camera, &evtFrame, EVT_INFINITE);
    if(err) 
      printf("EVT_CameraGetFrame Error = %8.8x!\n", err);
    else
      frames_recd++;

    //Tell camera to stop streaming before we process frames and switch pixel format.
    EVT_CameraExecuteCommand(&camera, "AcquisitionStop");

    if(     strcmp(enumMember, "Mono8") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      int a = 0;
      //while (true)
     //{

    
      sprintf_s(filename, "D:/images/myimageA_%d_%d.tif", enum_count,a);
     // a = a + 1;
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif
//      }

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif
    }
    else if(strcmp(enumMember, "Mono10") == 0 || strcmp(enumMember, "Mono12") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimageB_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_LEFT); //EVT_FrameConvert not required.
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NONE); //8 bit for bmp.
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "Mono10Packed") == 0 || strcmp(enumMember, "Mono12Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NONE); //Need to call to unpack.
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE);
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimageC_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NONE); //Need to call to unpack.
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_LEFT);
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NONE); //8 bit for bmp and unpack.
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "BayerGB8") == 0 || strcmp(enumMember, "BayerGR8") == 0 || strcmp(enumMember, "BayerRG8") == 0 || strcmp(enumMember, "BayerBG8") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_BILINEAR_RGB); //Bayer interpolate
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_BILINEAR_BGR); //Bayer interpolate
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "BayerGB10") == 0 || strcmp(enumMember, "BayerGR10") == 0 || strcmp(enumMember, "BayerRG10") == 0 || strcmp(enumMember, "BayerBG10") == 0 || strcmp(enumMember, "BayerGB12") == 0 || strcmp(enumMember, "BayerGR12") == 0 || strcmp(enumMember, "BayerRG12") == 0 || strcmp(enumMember, "BayerBG12") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_BILINEAR_RGB); //Bayer interpolate
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_BILINEAR_BGR); //Bayer interpolate and 8 bit.
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "RGB8Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR);  //BGR for bmp
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "RGB10Packed") == 0 || strcmp(enumMember, "RGB12Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_TO_BGR);  //8 bit for bmp
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "BGR8Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_RGB); //RGB for TIF
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif
    }
    else if(strcmp(enumMember, "BGR10Packed") == 0 || strcmp(enumMember, "BGR12Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE); //EVT_FrameConvert not required.
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_RGB); //RGB for TIF
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_8BIT, EVT_COLOR_CONVERT_NONE);   //8 bit for bmp
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "YUV411Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NONE);   //yuv unpack
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE);
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_RGB); //yuv unpack, yuv->rgb
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR); //yuv unpack, yuv->bgr
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE);
#endif
    }
    else if(strcmp(enumMember, "YUV422Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NONE);   //yuv unpack
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE);
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_RGB); //yuv unpack, yuv->rgb
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE);
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR); //yuv unpack, yuv->bgr
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE); 
#endif
    }
    else if(strcmp(enumMember, "YUV444Packed") == 0)
    {
#ifdef RAW_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.raw", enum_count);
#else
      sprintf(filename, "myimage_%d.raw", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_NONE);   //yuv unpack
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_RAW, EVT_ALIGN_NONE);
#endif

#ifdef TIF_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.tif", enum_count);
#else
      sprintf(filename, "myimage_%d.tif", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_RGB); //yuv unpack, yuv->rgb
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); 
#endif

#ifdef BMP_FILE_SAVE
#ifdef _MSC_VER
      sprintf_s(filename, "myimage_%d.bmp", enum_count);
#else
      sprintf(filename, "myimage_%d.bmp", enum_count);
#endif
      EVT_FrameConvert(&evtFrame, &evtFrameConvert, EVT_CONVERT_NONE, EVT_COLOR_CONVERT_TO_BGR); //yuv unpack, yuv->bgr
      EVT_FrameSave(&evtFrameConvert, filename, EVT_FILETYPE_BMP, EVT_ALIGN_NONE); 
#endif
    }
	  
    //Get next member
    enumMember = strtok_s(NULL, ",", &next_token); 
    if(enumMember)
    {
      enum_count++;
      EVT_CameraSetEnumParam(&camera, "PixelFormat", enumMember);
      printf("PixelFormat: \t\t%d %s\n", enum_count, enumMember);
    }
  } 

  printf("\nImages Captured: \t%d\n", frames_recd);

  //Release frame buffers
  EVT_ReleaseFrameBuffer(&camera, &evtFrame);
  EVT_ReleaseFrameBuffer(&camera, &evtFrameConvert);
  
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


