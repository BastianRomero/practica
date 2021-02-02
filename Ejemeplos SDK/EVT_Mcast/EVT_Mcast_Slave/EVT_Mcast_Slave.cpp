/*****************************************************************************
 * EVT_Mcast_Slave.cpp
 ***************************************************************************** 
 *
 * Project:      EVT_Mcast_Slave
 * Description:  Example program
 *
 * (c) 2019      by Emergent Vision Technologies Inc
 *               www.emergentvisiontec.com
 *****************************************************************************
 *   The EVT_Mcast_Slave example starts multicast stream in slave mode
 *   to receive frames from camera which is controlled by the master.
 *   Command line usage:
 *   EVT_Mcast_Slave <PixelFormat> <width> <height> <NIC ip> <multicast ip> <multicast port> [CPU]
 *   ie.
 *   EVT_Mcast_Slave BayerRG8 4096 3000 192.168.1.40 239.255.255.250 60646 0
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
#define FRAMES_BUFFERS 20

char* next_token;
int cpuToUse;

int main(int argc, char *argv[])
{
  CEmergentCamera camera;
  int ReturnVal = SUCCESS;
  unsigned int height_max, width_max;
  CEmergentFrame evtFrame[FRAMES_BUFFERS], evtFrameRecv;
  EVT_ERROR err = EVT_SUCCESS;
  char* pixelFormat;
  char* ifaceIp;
  char* multicastIp;
  unsigned short multicastPort;



  printf("-------------------------------"); printf("\n");
  printf("Mcast_Slave : Example program  "); printf("\n");
  printf("-------------------------------"); printf("\n");

#ifdef _MSC_VER  
  //Set the CPU to run on if specified on command line.
  if(argc == 8)
  {
    cpuToUse = atoi(argv[7]); // 1 the first cpu, 2 the second cpu
    if(cpuToUse > 0)
    {
      DWORD_PTR cpuMask = 1 << (cpuToUse - 1);
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
  if(argc == 8)
  {
    cpuToUse = atoi(argv[7]); // 1 the first cpu, 2 the second cpu
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

  if(argc >= 7)
  {
    pixelFormat = argv[1];
    width_max = atoi(argv[2]);
    height_max = atoi(argv[3]);
    ifaceIp = argv[4];
    multicastIp = argv[5];
    multicastPort = atoi(argv[6]);
    printf("Pixel Format: %s, Width: %d, Height: %d, iface Ip: %s, Multicast Ip: %s, port: %d.\n", pixelFormat, width_max, height_max, ifaceIp, multicastIp, multicastPort);
  }
  else
  {
    printf("Not enough arguments. Exiting program.\n");
    return 0; 
  }

  camera.ifaceAddress = ifaceIp;
  camera.multicastAddress = multicastIp;
  camera.portMulticast = multicastPort;
  
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

    if(strcmp(pixelFormat, "Mono8") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_MONO8;
    }
    else if(strcmp(pixelFormat, "Mono10") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_MONO10;
    }
    else if(strcmp(pixelFormat, "Mono10Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_MONO10_PACKED;
    }
	  else if(strcmp(pixelFormat, "Mono12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_MONO12;
	  }
	  else if(strcmp(pixelFormat, "Mono12Packed") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_MONO12_PACKED;
	  }
    else if(strcmp(pixelFormat, "BayerGB8") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB8;
    }
    else if(strcmp(pixelFormat, "BayerGR8") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR8;
    }
    else if(strcmp(pixelFormat, "BayerRG8") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG8;
    }
	  else if(strcmp(pixelFormat, "BayerBG8") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG8;
	  }
	  else if(strcmp(pixelFormat, "BayerGB10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB10;
	  }
	  else if(strcmp(pixelFormat, "BayerGR10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR10;
	  }
	  else if(strcmp(pixelFormat, "BayerRG10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG10;
	  }
	  else if(strcmp(pixelFormat, "BayerBG10") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG10;
	  }
	  else if(strcmp(pixelFormat, "BayerGB12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGB12;
	  }
	  else if(strcmp(pixelFormat, "BayerGR12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYGR12;
	  }
	  else if(strcmp(pixelFormat, "BayerRG12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYRG12;
	  }
	  else if(strcmp(pixelFormat, "BayerBG12") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG12;
	  }
	  else if (strcmp(pixelFormat, "BayerBG12Packed") == 0)
	  {
		  evtFrame[frame_count].pixel_type = GVSP_PIX_BAYBG12_PACKED;
	  }
    else if(strcmp(pixelFormat, "RGB8Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_RGB8;
    }
    else if(strcmp(pixelFormat, "RGB10Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_RGB10;
    }
	  else if(strcmp(pixelFormat, "RGB12Packed") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_RGB12;
	  }
    else if(strcmp(pixelFormat, "BGR8Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_BGR8;
    }
    else if(strcmp(pixelFormat, "BGR10Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_BGR10;
    }
	  else if(strcmp(pixelFormat, "BGR12Packed") == 0)
	  {
		evtFrame[frame_count].pixel_type = GVSP_PIX_BGR12;
	  }
    else if(strcmp(pixelFormat, "YUV411Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_YUV411_PACKED;
    }
    else if(strcmp(pixelFormat, "YUV422Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_YUV422_PACKED;
    }
    else if(strcmp(pixelFormat, "YUV444Packed") == 0)
    {
      evtFrame[frame_count].pixel_type = GVSP_PIX_YUV444_PACKED;
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
  double time_diff = (double(end-begin))/CLOCKS_PER_SEC;
#else
  gettimeofday(&endTime, NULL);
  float time_diff = (endTime.tv_sec  - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec)/1000000.0;
#endif


  //Report stats
  printf("\n");
  printf("Images Captured: \t%d\n", frames_recd);
  printf("Dropped Frames: \t%d\n", dropped_frames);
  printf("Calculated Frame Rate: \t%f\n", frames_recd/time_diff);

  //Release frame buffers
	for(int frame_count=0;frame_count<FRAMES_BUFFERS;frame_count++)
	{
		EVT_ReleaseFrameBuffer(&camera, &evtFrame[frame_count]);
	}

	//Host side tear down for stream.
	EVT_CameraCloseStream(&camera);

  printf("\nClose Stream: \t\tStream Closed\n");

  return(0);
} 
