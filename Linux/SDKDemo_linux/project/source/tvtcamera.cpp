<<<<<<< HEAD
// SDKDemo.cpp : 定义控制台应用程序的入口点。
///
#include <stdlib.h>
#include <string.h>
//#include "DeviceManager.h"
#include "DVR_NET_SDK.h"
#include "Typedef.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <cstddef>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "time.h"

#include<fstream>
#include<iostream>

// open source ini parser
#include "iniparser.h"
#include "dictionary.h"
// open source zlog
#include "zlog.h"

#define USE_CAMERA 1


#define USE_MODBUS 1
#if (USE_MODBUS == 1)
#include "modbus.h"
#include "unit-test.h"
#endif

#define LOGIN_OUT_TEST 0

char NUM_CAMARAS_MAX  = 120; //2020.04 for modbus TCP suport <= 123
char NUM_CAMARAS  = 90;

unsigned int IP_VALID_START  = 100;
unsigned int  IP_VALID_END  = NUM_CAMARAS + IP_VALID_START - 1;

BOOL bNeedConnect[255];
unsigned int arrayLoginID[255];
char cameraIP[255][16] = {0};

void excertionCallback( DWORD dwType, LONG lUserID, LONG lHandle, void *pUser )
{
    printf("\r\ntype:%d, userID:%d, handle:%d---NETWORK_DISCONNECT\r\n", dwType, lUserID, lHandle);
}

BOOL msgCallback( LONG lCommand, LONG lUserID, char *pBuf, DWORD dwBufLen, void *pUser )
{
    printf("msgCallback lCommand :%d, \n", lCommand);
    return 0;
}

void AcceptRegisterProc(LONG lUserID, LONG lRegisterID, LPNET_SDK_DEVICEINFO pDeviceInfo, void *pUser)
{

    printf("##########################\n\n");
    printf("AcceptRegisterProc lUserID=%d, lRegisterID=%d \n", lUserID, lRegisterID);
    printf("\n##########################\n");
}

int camera_online(int start, int end)
{
    int sum=0;
    for(int i = start; i < end+1;  i++)
    {
        if(FALSE == bNeedConnect[i])
            sum += 1;
    }
    return sum;
}

int device_capture(LONG index, const char *arg)
{
    int ret = 0;
    char filename[64] = {0};
    char cameraStatus = 0;
    BOOL bResult = FALSE;

    long deviceID = arrayLoginID[index];
    long channel = 0;

    zlog_category_t *plog=0;

    sprintf(filename, "./data/%s.jpg",arg);

    //printf("current deviceID = %d, channel = %d, start capture \n",deviceID, channel);

    DWORD dwRetJpegLen = 0;
    const int JPEG_DATA_LEN = 1920*1080*5;
    char *pJPEGBuf = new char[JPEG_DATA_LEN];
    if (pJPEGBuf)
    {
        memset(pJPEGBuf, 0x00, JPEG_DATA_LEN);
        //if ((bResult = m_pDeviceMan->CaptureJPEGData(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen)))
        //if ((bResult = m_pDeviceMan->CaptureJPEGData(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen))) // addedd by EHL 20191014
        if ((bResult = NET_SDK_CaptureJPEGData_V2(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen))) // @201911
        {

            sprintf(filename, "./data/%s.jpg",cameraIP[index]);

            FILE* fp = fopen(filename, "wb");
            if (fp)
            {
                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                fclose(fp);
            }
            cameraStatus = 1;
            printf("save jpg success!\n");
            printf("dwRetJpegLen = %d, get jpg success. \n",dwRetJpegLen);
            bNeedConnect[index] = FALSE;

        }
        else
        {
            printf("faild to capture jpeg data!\n");
            ret = NET_SDK_GetLastError();

#if (LOG_IN_FILE == 1)
            plog = zlog_get_category("my_cat");
            if (!plog) {
                printf("get cat fail\n");
                zlog_fini();
            }
            else
            {
                zlog_info(plog,"camera capture: IP=%s, NET_SDK_GetLastError=%d, dwRetJpegLen=%d, bResult=%d\n",cameraIP[index], ret,dwRetJpegLen, bResult);
            }
#endif
            printf("Logout the device[%ld].\n",deviceID);
            NET_SDK_Logout(deviceID);
            ret = NET_SDK_GetLastError();
            sprintf(pJPEGBuf, "[%s] line:%d  NET_SDK_GetLastError:%d failed!\n", __FILE__, __LINE__, ret);
            printf("%s ",pJPEGBuf);
            bNeedConnect[deviceID] = TRUE;

            cameraStatus = 0;
        }

        delete []pJPEGBuf;
        pJPEGBuf = NULL;
    }
    return cameraStatus;
}


int capture_pic(long index)
{
    if(bNeedConnect[index] == TRUE)
    {
        if(1 == running_debug)
           printf("bNeedConnect=1, exist capture!\n");
        return 0;
    }
    else if(index < 255)
    {
        if(device_capture(index, cameraIP[index]))
        {
            return 1;
        }
        else
        {
            bNeedConnect[index] = TRUE; //2019.11.26
            return 0;
        }
    }
    else
        return 0;
}


void *camera_thread(void *arg)
{
    int i = 0;
    int sum = 0;
    int connectTimeout = 100;
    BOOL m_bConnecting = 1;
	char sDVRIP[64];
	unsigned short nPort;
	char sUsername[64];
    char sPassword[64];
	NET_SDK_DEVICEINFO deviceInfo;


    zlog_category_t *plog = zlog_get_category("my_cat");
    if (!plog) {
        printf("get cat fail\n");
        zlog_fini();
    }
    else
    {
        zlog_debug(plog,"camera_thread");
    }

    while(m_bConnecting)
	{
        //NET_SDK_SetConnectTime(connectTimeout, 1);
        for(i = IP_VALID_START; i < IP_VALID_END+1; i ++)
        {
            strcpy(sDVRIP, cameraIP[i]);
            nPort = 9008;
            strcpy(sUsername, "admin");
            strcpy(sPassword, "123456");

            if(bNeedConnect[i])
            {
                printf("camera_thread : Login the camera[%d] %s ...\n",i,sDVRIP);

                LONG lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
                /*
                printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                    lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
                */
                printf("lLoginID:%d, deviceID=%ld,  deviceName=%s\n\n",lLoginID, deviceInfo.deviceID, deviceInfo.deviceName);

                if(lLoginID > 0)
                {
                    arrayLoginID[i] = lLoginID;
                    bNeedConnect[i] = FALSE;
                    printf("Camera thread: bNeedConnect[%d]=%d,  Camera status OK.\n\n",i,bNeedConnect[i]);
                }
                else
                {
                    //printf("bNeedConnect[%d]=%d ...\n\n",i,bNeedConnect[i]);
                    bNeedConnect[i] = TRUE;
                    unsigned int ret = NET_SDK_GetLastError();
                   // printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);

#if (LOG_IN_FILE == 1)
                    plog = zlog_get_category("my_cat");
                    if (!plog) {
                        printf("get cat fail\n");
                        zlog_fini();
                    }
                    else
                    {
                        zlog_info(plog,"ERROR login camera IP %s, NET_SDK_GetLastError=%d \n",sDVRIP, ret);
                    }
#endif
                    usleep(connectTimeout*1000);
                }

                // print the number of cameras exist.
                printf("Camera thread: %d of %d cameras exist.\n\n",camera_online(IP_VALID_START,IP_VALID_END), IP_VALID_END-IP_VALID_START + 1);


            }// end of if

        } // end of for

        if(connectTimeout < 6400)
        {
            connectTimeout *= 2;
        }

        //分多次Sleep，以便在线程退出时能快速完成
		int sleepedTime = 0;
		while(m_bConnecting)
		{
            //PUB_Sleep(100);
            usleep(100);
			sleepedTime += 100;
            if(sleepedTime >= 2000)
			{
				break;
			}
		}
   }// end of outer while
   pthread_exit(NULL);
}

#if (LOGIN_OUT_TEST == 1)
void test_login_logout(void)
{
    unsigned int ret;
    LONG lLoginID=0;
    char sDVRIP[64];
    unsigned short nPort;
    char sUsername[64];
    char sPassword[64];
    int channelNum;
    NET_SDK_DEVICEINFO deviceInfo;

    DWORD dwRetJpegLen = 0;
    const int JPEG_DATA_LEN = 1024*1024*40;
    char *pJPEGBuf = new char[JPEG_DATA_LEN];
    char filename[64] = {0};
    char svrAddr[64];

    BOOL bResult = FALSE;
    long deviceID, channel;


    while(1)
    {

        int i=0;
        nPort = 9008;
        strcpy(sUsername, "admin");
        strcpy(sPassword, "123456");
        sprintf(sDVRIP, "192.168.0.%d",14);

        printf("now login %s\n",sDVRIP);
        lLoginID = 0;
        while(lLoginID <= 0)
        {
            lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
            printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            if(lLoginID > 0)
            {
                arrayLoginID[i] = lLoginID;
                bNeedConnect[i] = 0;
                printf("deviceID = %d,  deviceName = %s, localVideoInputNum = %d, videoInputNum = %d, videoOutputNum = %d", \
                deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            }
            else
            {
                bNeedConnect[i] = 1;
                ret = NET_SDK_GetLastError();
                printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
            }
        }
        arrayLoginID[0] = lLoginID;

/*      sprintf(sDVRIP, "192.168.0.%d",253);

        lLoginID = 0;
        i=1;
        while(lLoginID <= 0)
        {
            lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
            printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            if(lLoginID > 0)
            {
                arrayLoginID[i] = lLoginID;
                bNeedConnect[i] = FALSE;
                printf("deviceID = %d,  deviceName = %s, localVideoInputNum = %d, videoInputNum = %d, videoOutputNum = %d\n", \
                deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            }
            else
            {
                bNeedConnect[i] = TRUE;
                ret = NET_SDK_GetLastError();
                printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
            }
        }*/

        capture_pic(0);

        sleep(10);
        printf("\n");
        printf("now logout the camera.\n");
        ret = NET_SDK_Logout(arrayLoginID[0]);
        printf("camera 1 logout, ret = %d\n",ret);

        printf("waiting ...\n");
        NET_SDK_Cleanup();
        sleep(61);
        NET_SDK_Init();
        NET_SDK_SetConnectTime();
        NET_SDK_SetReconnect(10000, TRUE);
    }
}
#endif


=======
// SDKDemo.cpp : 定义控制台应用程序的入口点。
///
#include <stdlib.h>
#include <string.h>
//#include "DeviceManager.h"
#include "DVR_NET_SDK.h"
#include "Typedef.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <cstddef>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "time.h"

#include<fstream>
#include<iostream>

// open source ini parser
#include "iniparser.h"
#include "dictionary.h"
// open source zlog
#include "zlog.h"

#define USE_CAMERA 1


#define USE_MODBUS 1
#if (USE_MODBUS == 1)
#include "modbus.h"
#include "unit-test.h"
#endif

#define LOGIN_OUT_TEST 0

char NUM_CAMARAS_MAX  = 120; //2020.04 for modbus TCP suport <= 123
char NUM_CAMARAS  = 90;

unsigned int IP_VALID_START  = 100;
unsigned int  IP_VALID_END  = NUM_CAMARAS + IP_VALID_START - 1;

BOOL bNeedConnect[255];
unsigned int arrayLoginID[255];
char cameraIP[255][16] = {0};

void excertionCallback( DWORD dwType, LONG lUserID, LONG lHandle, void *pUser )
{
    printf("\r\ntype:%d, userID:%d, handle:%d---NETWORK_DISCONNECT\r\n", dwType, lUserID, lHandle);
}

BOOL msgCallback( LONG lCommand, LONG lUserID, char *pBuf, DWORD dwBufLen, void *pUser )
{
    printf("msgCallback lCommand :%d, \n", lCommand);
    return 0;
}

void AcceptRegisterProc(LONG lUserID, LONG lRegisterID, LPNET_SDK_DEVICEINFO pDeviceInfo, void *pUser)
{

    printf("##########################\n\n");
    printf("AcceptRegisterProc lUserID=%d, lRegisterID=%d \n", lUserID, lRegisterID);
    printf("\n##########################\n");
}

int camera_online(int start, int end)
{
    int sum=0;
    for(int i = start; i < end+1;  i++)
    {
        if(FALSE == bNeedConnect[i])
            sum += 1;
    }
    return sum;
}

int device_capture(LONG index, const char *arg)
{
    int ret = 0;
    char filename[64] = {0};
    char cameraStatus = 0;
    BOOL bResult = FALSE;

    long deviceID = arrayLoginID[index];
    long channel = 0;

    zlog_category_t *plog=0;

    sprintf(filename, "./data/%s.jpg",arg);

    //printf("current deviceID = %d, channel = %d, start capture \n",deviceID, channel);

    DWORD dwRetJpegLen = 0;
    const int JPEG_DATA_LEN = 1920*1080*5;
    char *pJPEGBuf = new char[JPEG_DATA_LEN];
    if (pJPEGBuf)
    {
        memset(pJPEGBuf, 0x00, JPEG_DATA_LEN);
        //if ((bResult = m_pDeviceMan->CaptureJPEGData(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen)))
        //if ((bResult = m_pDeviceMan->CaptureJPEGData(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen))) // addedd by EHL 20191014
        if ((bResult = NET_SDK_CaptureJPEGData_V2(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen))) // @201911
        {

            sprintf(filename, "./data/%s.jpg",cameraIP[index]);

            FILE* fp = fopen(filename, "wb");
            if (fp)
            {
                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                fclose(fp);
            }
            cameraStatus = 1;
            printf("save jpg success!\n");
            printf("dwRetJpegLen = %d, get jpg success. \n",dwRetJpegLen);
            bNeedConnect[index] = FALSE;

        }
        else
        {
            printf("faild to capture jpeg data!\n");
            ret = NET_SDK_GetLastError();

#if (LOG_IN_FILE == 1)
            plog = zlog_get_category("my_cat");
            if (!plog) {
                printf("get cat fail\n");
                zlog_fini();
            }
            else
            {
                zlog_info(plog,"camera capture: IP=%s, NET_SDK_GetLastError=%d, dwRetJpegLen=%d, bResult=%d\n",cameraIP[index], ret,dwRetJpegLen, bResult);
            }
#endif
            printf("Logout the device[%ld].\n",deviceID);
            NET_SDK_Logout(deviceID);
            ret = NET_SDK_GetLastError();
            sprintf(pJPEGBuf, "[%s] line:%d  NET_SDK_GetLastError:%d failed!\n", __FILE__, __LINE__, ret);
            printf("%s ",pJPEGBuf);
            bNeedConnect[deviceID] = TRUE;

            cameraStatus = 0;
        }

        delete []pJPEGBuf;
        pJPEGBuf = NULL;
    }
    return cameraStatus;
}


int capture_pic(long index)
{
    if(bNeedConnect[index] == TRUE)
    {
        if(1 == running_debug)
           printf("bNeedConnect=1, exist capture!\n");
        return 0;
    }
    else if(index < 255)
    {
        if(device_capture(index, cameraIP[index]))
        {
            return 1;
        }
        else
        {
            bNeedConnect[index] = TRUE; //2019.11.26
            return 0;
        }
    }
    else
        return 0;
}


void *camera_thread(void *arg)
{
    int i = 0;
    int sum = 0;
    int connectTimeout = 100;
    BOOL m_bConnecting = 1;
	char sDVRIP[64];
	unsigned short nPort;
	char sUsername[64];
    char sPassword[64];
	NET_SDK_DEVICEINFO deviceInfo;


    zlog_category_t *plog = zlog_get_category("my_cat");
    if (!plog) {
        printf("get cat fail\n");
        zlog_fini();
    }
    else
    {
        zlog_debug(plog,"camera_thread");
    }

    while(m_bConnecting)
	{
        //NET_SDK_SetConnectTime(connectTimeout, 1);
        for(i = IP_VALID_START; i < IP_VALID_END+1; i ++)
        {
            strcpy(sDVRIP, cameraIP[i]);
            nPort = 9008;
            strcpy(sUsername, "admin");
            strcpy(sPassword, "123456");

            if(bNeedConnect[i])
            {
                printf("camera_thread : Login the camera[%d] %s ...\n",i,sDVRIP);

                LONG lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
                /*
                printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                    lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
                */
                printf("lLoginID:%d, deviceID=%ld,  deviceName=%s\n\n",lLoginID, deviceInfo.deviceID, deviceInfo.deviceName);

                if(lLoginID > 0)
                {
                    arrayLoginID[i] = lLoginID;
                    bNeedConnect[i] = FALSE;
                    printf("Camera thread: bNeedConnect[%d]=%d,  Camera status OK.\n\n",i,bNeedConnect[i]);
                }
                else
                {
                    //printf("bNeedConnect[%d]=%d ...\n\n",i,bNeedConnect[i]);
                    bNeedConnect[i] = TRUE;
                    unsigned int ret = NET_SDK_GetLastError();
                   // printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);

#if (LOG_IN_FILE == 1)
                    plog = zlog_get_category("my_cat");
                    if (!plog) {
                        printf("get cat fail\n");
                        zlog_fini();
                    }
                    else
                    {
                        zlog_info(plog,"ERROR login camera IP %s, NET_SDK_GetLastError=%d \n",sDVRIP, ret);
                    }
#endif
                    usleep(connectTimeout*1000);
                }

                // print the number of cameras exist.
                printf("Camera thread: %d of %d cameras exist.\n\n",camera_online(IP_VALID_START,IP_VALID_END), IP_VALID_END-IP_VALID_START + 1);


            }// end of if

        } // end of for

        if(connectTimeout < 6400)
        {
            connectTimeout *= 2;
        }

        //分多次Sleep，以便在线程退出时能快速完成
		int sleepedTime = 0;
		while(m_bConnecting)
		{
            //PUB_Sleep(100);
            usleep(100);
			sleepedTime += 100;
            if(sleepedTime >= 2000)
			{
				break;
			}
		}
   }// end of outer while
   pthread_exit(NULL);
}

#if (LOGIN_OUT_TEST == 1)
void test_login_logout(void)
{
    unsigned int ret;
    LONG lLoginID=0;
    char sDVRIP[64];
    unsigned short nPort;
    char sUsername[64];
    char sPassword[64];
    int channelNum;
    NET_SDK_DEVICEINFO deviceInfo;

    DWORD dwRetJpegLen = 0;
    const int JPEG_DATA_LEN = 1024*1024*40;
    char *pJPEGBuf = new char[JPEG_DATA_LEN];
    char filename[64] = {0};
    char svrAddr[64];

    BOOL bResult = FALSE;
    long deviceID, channel;


    while(1)
    {

        int i=0;
        nPort = 9008;
        strcpy(sUsername, "admin");
        strcpy(sPassword, "123456");
        sprintf(sDVRIP, "192.168.0.%d",14);

        printf("now login %s\n",sDVRIP);
        lLoginID = 0;
        while(lLoginID <= 0)
        {
            lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
            printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            if(lLoginID > 0)
            {
                arrayLoginID[i] = lLoginID;
                bNeedConnect[i] = 0;
                printf("deviceID = %d,  deviceName = %s, localVideoInputNum = %d, videoInputNum = %d, videoOutputNum = %d", \
                deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            }
            else
            {
                bNeedConnect[i] = 1;
                ret = NET_SDK_GetLastError();
                printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
            }
        }
        arrayLoginID[0] = lLoginID;

/*      sprintf(sDVRIP, "192.168.0.%d",253);

        lLoginID = 0;
        i=1;
        while(lLoginID <= 0)
        {
            lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
            printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            if(lLoginID > 0)
            {
                arrayLoginID[i] = lLoginID;
                bNeedConnect[i] = FALSE;
                printf("deviceID = %d,  deviceName = %s, localVideoInputNum = %d, videoInputNum = %d, videoOutputNum = %d\n", \
                deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            }
            else
            {
                bNeedConnect[i] = TRUE;
                ret = NET_SDK_GetLastError();
                printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
            }
        }*/

        capture_pic(0);

        sleep(10);
        printf("\n");
        printf("now logout the camera.\n");
        ret = NET_SDK_Logout(arrayLoginID[0]);
        printf("camera 1 logout, ret = %d\n",ret);

        printf("waiting ...\n");
        NET_SDK_Cleanup();
        sleep(61);
        NET_SDK_Init();
        NET_SDK_SetConnectTime();
        NET_SDK_SetReconnect(10000, TRUE);
    }
}
#endif


>>>>>>> a6d2214bda1c223f2b5a1caf2eccb7f94d6903ae
