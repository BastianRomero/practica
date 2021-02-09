/*****************************************************************************
 * EVT_GPIO.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_GPIO
 * Description:  Example program
 *
 * (c) 2012      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_GPIO example opens the device and configures
 *   the camera for external triggering and receives frames
 *   via said external triggers.
 *   This example requires external triggering signals on
 *   a GPI pin to function.
 *   Thus it exercises functionality of the XML GPIO group.
 ****************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
  #include <windows.h>
#else
  #include <unistd.h>
#endif
#include <EmergentCameraAPIs.h>
#include <emergentframe.h>
#include <EvtParamAttribute.h>
#include <gigevisiondeviceinfo.h>
#include <EmergentFrameSave.h>

using namespace std;
using namespace Emergent;

#define SUCCESS 0
//#define XML_FILE   "C:\\xml\\Emergent_HS-20000-C_1_0.xml"
#define MAX_FRAMES 150

#define POLARITY_NEG 1
#define POLARITY_POS 0

#define TRUE  1
#define FALSE 0

#define EXPOSURE_VAL 1000 //us

#define MAX_CAMERAS 10

void configure_defaults(CEmergentCamera* camera);
char* next_token;

int main(int argc, char* argv[])
{
  CEmergentCamera camera;
  int ReturnVal = SUCCESS;
  CEmergentFrame evtFrame;
  unsigned int height_max, width_max, param_val_max, param_val_min;
  bool gpo_polarity = TRUE;
  char gpo_str[20];
  char filename[100];
  struct GigEVisionDeviceInfo deviceInfo[MAX_CAMERAS];
  unsigned int count, camera_index;
  EVT_ERROR err = EVT_SUCCESS;

  printf("------------------------"); printf("\n");
  printf("GPIO : Example program  "); printf("\n");
  printf("------------------------"); printf("\n");

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

  //Test GPOs by toggling polarity in manual mode.
  for(count=0;count<6;count++)
  {

#ifdef _MSC_VER
    sprintf_s(gpo_str, "GPO_%d_Mode", count);
    //ej1:sprintf_s(gpo_str, "GPO_%d_Polarity", count);
#else
    sprintf(gpo_str, "GPO_%d_Mode", count);
#endif

    EVT_CameraSetEnumParam(&camera, gpo_str, "GPO");
    //ej1:EVT_CameraSetEnumParam(&camera, gpo_str, "positive or negative");

    // GPO es un modo de la salida de los pines que pueden enviar señales.
  }

  for(count=0;count<6;count++)
  {
    printf("Toggling GPO %d\t\t", count);

#ifdef _MSC_VER
    sprintf_s(gpo_str, "GPO_%d_Polarity", count);
#else
    sprintf(gpo_str, "GPO_%d_Polarity", count);
#endif

    for(int blink_count=0;blink_count<20;blink_count++)
    {
      EVT_CameraSetBoolParam(&camera, gpo_str, gpo_polarity);
      gpo_polarity = !gpo_polarity;
#ifdef _MSC_VER
      Sleep(100);
#else
	usleep(100 * 1000);
#endif
      printf(".");
#ifndef _MSC_VER 
      fflush(stdout);
#endif

    }
    printf("\n");
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
  evtFrame.pixel_type = GVSP_PIX_MONO8;  //Covers color model using BayerGB8 also.
  EVT_AllocateFrameBuffer(&camera, &evtFrame, EVT_FRAME_BUFFER_ZERO_COPY);
  EVT_CameraQueueFrame(&camera, &evtFrame);

  printf("Enter <h> if external hardware trigger available on GPI4 or loopback GPO_0 to GPI_4. <s> to skip test. Then press enter: ");
  char user_char = getchar();

  //Clear enter key pressed from getchar buffer.
  getchar();

  //Setup continuous hardware trigger for MAX_FRAMES frame.
  if(user_char == 'h')
  {
    printf("Trigger Mode: \t\tContinuous, HardwareTrigger, GPI4, %d Frames\n", MAX_FRAMES);
    printf("Grabbing Frames...\n");

    EVT_CameraSetEnumParam(&camera, "TriggerMode", "On");
    //pagina 9 attribute_manual
    EVT_CameraSetEnumParam(&camera, "TriggerSource", "Hardware");
    //pagina 10 attribute_manual

    //Set the GPI hardware triggering mode to use GPI_4 and select rising edge to start exp and falling edge 
    //to end exposure. Error check omitted for clarity.
    EVT_CameraSetEnumParam(&camera, "GPI_Start_Exp_Mode",   "GPI_4");
    EVT_CameraSetEnumParam(&camera, "GPI_Start_Exp_Event",  "Rising_Edge");
    EVT_CameraSetEnumParam(&camera, "GPI_End_Exp_Mode",     "GPI_4");
    EVT_CameraSetEnumParam(&camera, "GPI_End_Exp_Event",    "Falling_Edge");

    EVT_CameraSetEnumParam(&camera, "GPO_0_Mode", "Test_Generator");
    EVT_CameraSetUInt32Param(&camera, "TG_Frame_Time", 33333); //30fps
    EVT_CameraSetUInt32Param(&camera, "TG_High_Time", 1000);   //1000us

    EVT_CameraSetUInt32Param(&camera, "Trigger_Delay", 1000);   //1000us

	  //Grab MAX_FRAMES frames and just save images to .bmp file.
    unsigned int frames_recd = 0;
	  for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	  {
	    //Tell camera to start streaming.
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


#ifdef _MSC_VER
      sprintf_s(filename, "myimage1_%d.tif", frame_count);
#else
      sprintf(filename, "myimage1_%d.tif", frame_count);
#endif

      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); 

      EVT_CameraQueueFrame(&camera, &evtFrame);  //Now re-queue.

      printf(".");
#ifndef _MSC_VER 
      fflush(stdout);
#endif
	  }

    printf("\nImages Captured: \t%d\n", frames_recd);
  }
  else
  {
    printf("External hardware trigger required on GPI4 to run test.\n");
  }
  

  printf("Enter <h> if external hardware trigger available on GPI4 or loopback GPO_0 to GPI_4. <s> to skip test. Then press enter: ");
  user_char = getchar();

  //Clear enter key pressed from getchar buffer.
  getchar();

  //Setup continuous hardware trigger for MAX_FRAMES frame.
  if(user_char == 'h')
  {
    printf("Trigger Mode: \t\tContinuous, HardwareTrigger, Internal Exp End, GPI4, %d Frames\n", MAX_FRAMES);
    printf("Grabbing Frames...\n");

    //Set exposure. Error check omitted for clarity.
    EVT_CameraGetUInt32ParamMax(&camera, "Exposure", &param_val_max);
    printf("Exposure Max: \t\t%d\n", param_val_max);
    EVT_CameraGetUInt32ParamMin(&camera, "Exposure", &param_val_min);
    printf("Exposure Min: \t\t%d\n", param_val_min);
    if(EXPOSURE_VAL >= param_val_min && EXPOSURE_VAL <= param_val_max)
    {
      EVT_CameraSetUInt32Param(&camera, "Exposure", EXPOSURE_VAL);
      printf("Exposure Set: \t\t%d\n", EXPOSURE_VAL);
    }

    //All other settings same as previous GPI4 test.
    EVT_CameraSetEnumParam(&camera, "GPI_End_Exp_Mode",     "Internal");

    EVT_CameraSetUInt32Param(&camera, "Trigger_Delay", 1000);   //1000us

	  //Grab MAX_FRAMES frames and just save images to .bmp file.
    unsigned int frames_recd = 0;
	  for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	  {
	    //Tell camera to start streaming.
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

#ifdef _MSC_VER
      sprintf_s(filename, "myimage2_%d.tif", frame_count);
#else
      sprintf(filename, "myimage2_%d.tif", frame_count);
#endif

      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); 

      EVT_CameraQueueFrame(&camera, &evtFrame);  //Now re-queue.

      printf(".");
#ifndef _MSC_VER 
      fflush(stdout);
#endif
	  }

    printf("\nImages Captured: \t%d\n", frames_recd);
  }
  else
  {
    printf("External hardware trigger required on GPI4 to run test.\n");
  }


// Se repite con GPI5
  printf("Enter <h> if external hardware trigger available on GPI5 or loopback GPO_1 to GPI_5. <s> to skip test. Then press enter: ");
  user_char = getchar();

  //Clear enter key pressed from getchar buffer.
  getchar();

  //Setup continuous hardware trigger for MAX_FRAMES frame.
  if(user_char == 'h')
  {
    printf("Trigger Mode: \t\tContinuous, HardwareTrigger, GPI5, %d Frames\n", MAX_FRAMES);
    printf("Grabbing Frames...\n");

    EVT_CameraSetEnumParam(&camera, "TriggerMode", "On");
    EVT_CameraSetEnumParam(&camera, "TriggerSource", "Hardware");

    //Set the GPI hardware triggering mode to use GPI_5 and select rising edge to start exp and falling edge 
    //to end exposure. Error check omitted for clarity.
    EVT_CameraSetEnumParam(&camera, "GPI_Start_Exp_Mode",   "GPI_5");
    EVT_CameraSetEnumParam(&camera, "GPI_Start_Exp_Event",  "Rising_Edge");
    EVT_CameraSetEnumParam(&camera, "GPI_End_Exp_Mode",     "GPI_5");
    EVT_CameraSetEnumParam(&camera, "GPI_End_Exp_Event",    "Falling_Edge");

    EVT_CameraSetEnumParam(&camera, "GPO_1_Mode", "Test_Generator");
    EVT_CameraSetUInt32Param(&camera, "TG_Frame_Time", 33333); //30fps
    EVT_CameraSetUInt32Param(&camera, "TG_High_Time", 1000);   //1000us

    EVT_CameraSetUInt32Param(&camera, "Trigger_Delay", 1000);   //1000us

	  //Grab MAX_FRAMES frames and just save images to .bmp file.
    unsigned int frames_recd = 0;
	  for(int frame_count=0;frame_count<MAX_FRAMES;frame_count++)
	  {
	    //Tell camera to start streaming.
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

#ifdef _MSC_VER
      sprintf_s(filename, "myimage3_%d.tif", frame_count);
#else
      sprintf(filename, "myimage3_%d.tif", frame_count);
#endif

      EVT_FrameSave(&evtFrame, filename, EVT_FILETYPE_TIF, EVT_ALIGN_NONE); 

      EVT_CameraQueueFrame(&camera, &evtFrame);  //Now re-queue.

      printf(".");
#ifndef _MSC_VER 
      fflush(stdout);
#endif
	  }

    printf("\nImages Captured: \t%d\n", frames_recd);
  }
  else
  {
    printf("External hardware trigger required on GPI5 to run test.\n");
  }
  
  //Release frame buffer
  EVT_ReleaseFrameBuffer(&camera, &evtFrame);

	//Host side tear down for stream.
	EVT_CameraCloseStream(&camera);

  ////////////////////////////////////////////////////////////////////////////////
  //UART test start
  printf("Enter <h> if external loopback GPO_3 to GPI_5 for Uart loopback test. <s> to skip test. Then press enter: ");
  user_char = getchar();

  //Clear enter key pressed from getchar buffer.
  getchar();

  //Run Uart Loopback Test.
  if(user_char == 'h')
  {
    unsigned int uart_data;

    printf("Running Uart Loopback Test...\n");

    //Use GPO3 as Uart transmit
    EVT_CameraSetEnumParam(&camera, "GPO_3_Mode", "Test_Gen_Uart_Txd");

    //Enables receive Uart which will be using GPI5.
    EVT_CameraSetBoolParam(&camera, "UartEnable", TRUE);

    //Set Uart baud rate for tx and rx
    EVT_CameraSetEnumParam(&camera, "UartBaud", "B_9600");

    //Set Uart number of data bits.
    EVT_CameraSetUInt32Param(&camera, "UartDataBits", 8); 

    //Set Uart number of stop bits.
    EVT_CameraSetUInt32Param(&camera, "UartStopBits", 1); 

    printf("Enabled Uart and set for 9600 8N1.\n");
    printf("Transmitting data...\n");

    //Transmit some data for Uart tx. This data will loopback external to the camera and go back into camera Uart rx.
    for(uart_data=48; uart_data<58; uart_data++)
    {
      EVT_CameraSetUInt32Param(&camera, "UartTxData", uart_data); 
      printf("0x%2.2x ", uart_data);
    }

    //Wait until all data sent.
    while(1)
    {
      EVT_CameraGetUInt32Param(&camera, "UartTxFifoCnt", &uart_data);
      if(!uart_data) break; //When all data out of tx fifo then sent.
    }

    //Check to see if all data is in rx fifo.
    while(1)
    {
      EVT_CameraGetUInt32Param(&camera, "UartRxFifoCnt", &uart_data);
      if(uart_data == 10) break; //When all data into rx fifo then all rec'd.
    }

    printf("\nReceiving data...\n");

    //Now read the data back which is now in rx fifo.
    for(int i=0; i<10; i++)
    {
      EVT_CameraGetUInt32Param(&camera, "UartRxData", &uart_data);
      printf("0x%2.2x ", uart_data);
    }

    printf("\nUart Test Complete.\n");

  }
  else
  {
    printf("External hardware loopback required to run Uart test.\n");
  }



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

  EVT_CameraSetBoolParam(camera,      "LUTEnable", FALSE);
  EVT_CameraSetBoolParam(camera,      "AutoGain", FALSE);

  EVT_CameraSetBoolParam(camera,     "UartEnable", FALSE);
}
