<<<<<<< HEAD
// SDKDemo.cpp : 定义控制台应用程序的入口点。
//

//#include "DeviceManager.h"
#include "DVR_NET_SDK.h"
#include "Typedef.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <cstddef>
#include <sys/stat.h>
#define FILEPATH_MAX (80)
#define LOG_IN_FILE 1

#define SAVE_JPG_ALL 1

#define USE_CAMERA 0

#define USE_SOCKET 1
#if (USE_SOCKET == 1)  
#include <sockserver.h>
#endif


#define USE_MODBUS 1
#if (USE_MODBUS == 1)
#include "modbus.h"
#include "unit-test.h"
#endif


#define LOGIN_OUT_TEST 0

//#include "unit-test.h"

BOOL bCameraStatus[255];
char bNeedConnect[255];
unsigned int arrayLoginID[255];

//#define NUM_CAMARAS 1

#if (NUM_CAMARAS>1)
const char *cameraIP[NUM_CAMARAS] = {"192.168.0.249", "192.168.0.253"};
#else
const char *cameraIP[NUM_CAMARAS] = {"192.168.0.249"}; // 2019.11.11 test
#endif

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

int device_capture(LONG index, const char *arg)
{
    int i, ret = 0;
    char filename[64] = {0};
    char cameraStatus = 0;
    BOOL bResult = FALSE;

    long deviceID = arrayLoginID[index];

    long channel = 0;

    sprintf(filename, "./data/%s.jpg",arg);

#if 0
    // delete the old jpg
    if(access(filename, F_OK) == 0) // if file exist
    {
        if(unlink(filename)<0){
            printf("unlink(%s) error !\n",filename);
        }
        if(access(filename, F_OK) != 0)
        {
            //printf("delete (%s) OK !\n",filename);
        }
    }
#endif


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

        /*    FILE* fp = fopen(filename, "wb");
            if (fp)
            {
                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                fclose(fp);
            }
            cameraStatus = 1;
            printf("save jpg success!\n");*/


#if (SAVE_JPG_ALL == 1)
            time_t nSeconds;
            time(&nSeconds);
            struct tm * pTM;
            pTM = localtime(&nSeconds);
            
            /*if(index == 1)
            {
                sprintf(filename, "./%s/%02d-%02d-%02d.jpg",cameraIP[i],pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
            }
            else
            {
                sprintf(filename, "./253/%02d-%02d-%02d.jpg",pTM->tm_hour, pTM->tm_min, pTM->tm_sec); 
            }*/
            sprintf(filename, "./%s/%02d-%02d-%02d.jpg",cameraIP[index],pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
            


            FILE* fp = fopen(filename, "wb");
            if (fp)
            {
                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                fclose(fp);
            }
            cameraStatus = 1;
            printf("save jpg success!\n");
            //delete[] tempstr;

#endif

            printf("dwRetJpegLen = %d, get jpg success. \n",dwRetJpegLen);
            bCameraStatus[index] = 1;
            cameraStatus = 1;

        }
        else
        {
            printf("[%s] line:%d  NET_SDK_GetLastError:%d \n", __FILE__, __LINE__, NET_SDK_GetLastError());
            printf("dwRetJpegLen = %d, bResult = %d \n",dwRetJpegLen, bResult);
            printf("faild to capture jpeg data!\n");

#if (LOG_IN_FILE == 1)
            time_t nSeconds;
            time(&nSeconds);
            struct tm * pTM;
            pTM = localtime(&nSeconds);

            //char *tempstr = new char[2048];
            printf("save in file test.log !\n");
            memset(pJPEGBuf, 0x00, JPEG_DATA_LEN);
            FILE* pFile = fopen("./test.log", "a");
            if(pFile)
            {
                sprintf(pJPEGBuf, "%04d-%02d-%02d %02d:%02d:%02d device_capture: IP = %s, NET_SDK_GetLastError:%d !\n", \
                        pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday, \
                        pTM->tm_hour, pTM->tm_min, pTM->tm_sec, \
                        cameraIP[index], ret);
                fwrite(pJPEGBuf, sizeof(char), strlen(pJPEGBuf), pFile);

                fclose(pFile);
            }
            //delete[] tempstr;

#endif
            printf("Logout the device[%d].\n",deviceID);
            NET_SDK_Logout(deviceID);
            ret = NET_SDK_GetLastError();
            sprintf(pJPEGBuf, "[%s] line:%d  NET_SDK_GetLastError:%d failed!\n", __FILE__, __LINE__, ret);
            printf("%s ",pJPEGBuf);
           // bNeedConnect[deviceID] = TRUE;

            cameraStatus = 0;
        }

        delete []pJPEGBuf;
        pJPEGBuf = NULL;
    }
    return cameraStatus;
}


int capture_pic(long index)
{
    if(bNeedConnect[index] == 1)
    {
        printf("bNeedConnect=1, exist capture!\n");
    }
    else if(index < NUM_CAMARAS)
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


void * camera_thread(void *arg)
{
    int i = 0;
    int connectTimeout = 100;
	BOOL m_bConnecting = 1;
	unsigned int			arrayDeviceID[255];
    unsigned int			arrayChannel[255];
	char sDVRIP[64];
	unsigned short nPort;
	char sUsername[64];
	char sPassword[64];
	int channelNum;
	NET_SDK_DEVICEINFO deviceInfo;


    while(m_bConnecting)
	{
        NET_SDK_SetConnectTime(connectTimeout, 1);
        for(i = 0; i < NUM_CAMARAS; i ++)
        {
            sprintf(sDVRIP, cameraIP[i]);
            nPort = 9008;
            strcpy(sUsername, "admin");
            strcpy(sPassword, "123456");

            if(bNeedConnect[i])
            {
                //printf("Login the camera[%d] %s ...\n",i,cameraIP[i]);
                LONG lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
                //printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                 //   lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
                printf("lLoginID:%d, deviceID=%d,  deviceName=%s\n",lLoginID, deviceInfo.deviceID, deviceInfo.deviceName);
                if(lLoginID > 0)
                {
                    arrayLoginID[i] = lLoginID;
                    bNeedConnect[i] = 0;
                    printf("bNeedConnect[%d]=%d\n\n",i,bNeedConnect[i]);
                    //bCameraStatus[i] = TRUE;
                }
                else
                {
                    printf("bNeedConnect[%d]=%d, please check the IP: %s ...\n\n",i,bNeedConnect[i],cameraIP[i]);
                    bNeedConnect[i] = 1;
                   // unsigned int ret = NET_SDK_GetLastError();
                   // printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
                }
            }
        }

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

    }
}

int main(int argc, char * argv[])
{
    int i=0;
    int running_count = 0;

	char command;
	DWORD dwErrorCode = 0;

    // initilize
    for(i = 0; i < 255; i++)
    {
        bNeedConnect[i] = 1;
    }
    printf("main(): bNeedConnect[] = %d %d %d ...\n",bNeedConnect[0],bNeedConnect[1],bNeedConnect[2]);

    DIR *dirp;
    dirp = opendir("data");
    if(NULL==dirp)
          mkdir("data",0775);
    closedir(dirp);

#if (SAVE_JPG_ALL == 1)

    for(i = 0; i < 2; i++)
    {
        dirp = opendir(cameraIP[i]);
        if(NULL==dirp)
          mkdir(cameraIP[i], 0775);
        closedir(dirp);
    }
#endif

#if 1
    time_t nSeconds;
    time(&nSeconds);
    struct tm * pTM;
    pTM = localtime(&nSeconds);

    char *tempstr = new char[2048];
    printf("save in file test.log !\n");
    memset(tempstr, 0x00, 2048);
    FILE* pFile = fopen("./test.log", "a");
    if(pFile)
    {
        /* 系统日期和时间,格式: yyyymmddHHMMSS */
        sprintf(tempstr, "%04d-%02d-%02d %02d:%02d:%02d main() start! NUM_CAMARAS=%d\n", \
                pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday, \
                pTM->tm_hour, pTM->tm_min, pTM->tm_sec,NUM_CAMARAS);

        fwrite(tempstr, sizeof(char), strlen(tempstr), pFile);

        fclose(pFile);
    }
    delete[] tempstr;

#endif

    NET_SDK_Init();
	NET_SDK_SetConnectTime();
	NET_SDK_SetReconnect(10000, TRUE);

#if (USE_CAMERA == 1)
    // create socket thread
    pthread_t tid_camera;
    pthread_create(&tid_camera,NULL,camera_thread,NULL);
#endif

    unsigned int ret;
    LONG lLoginID=0;
    char sDVRIP[64];
    unsigned short nPort;
    char sUsername[64];
    char sPassword[64];
    int channelNum;
    NET_SDK_DEVICEINFO deviceInfo;


#if (LOGIN_OUT_TEST == 1)
    while(1)
    {

        int i=0;
        nPort = 9008;
        strcpy(sUsername, "admin");
        strcpy(sPassword, "123456");
        sprintf(sDVRIP, "192.168.0.%d",249);

        printf("now login 249\n");
        lLoginID = 0;
        while(lLoginID <= 0)
        {
            lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
            printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            if(lLoginID > 0)
            {
                arrayLoginID[i] = lLoginID;
                bNeedConnect[i] = FALSE;
                printf("deviceID = %d,  deviceName = %s, localVideoInputNum = %d, videoInputNum = %d, videoOutputNum = %d", \
                deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            }
            else
            {
                bNeedConnect[i] = TRUE;
                ret = NET_SDK_GetLastError();
                printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
            }
        }
        arrayLoginID[0] = lLoginID;

/*        sprintf(sDVRIP, "192.168.0.%d",253);

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

        sleep(20);
        printf("\n");
        printf("now logout the camera.\n");
        ret = NET_SDK_Logout(arrayLoginID[0]);
        printf("camera 1 logout, ret = %d\n",ret);

     /*   printf("now logout the camera.\n");
        ret = NET_SDK_Logout(2);
        printf("camera 2 logout, ret = %d\n",ret);*/

        printf("waiting ...\n");
       NET_SDK_Cleanup();
        sleep(61);
        NET_SDK_Init();
        NET_SDK_SetConnectTime();
        NET_SDK_SetReconnect(10000, TRUE);

    }
#endif


    //unsigned int ret;
    BOOL m_bConnecting = 1;

    DWORD dwRetJpegLen = 0;
    const int JPEG_DATA_LEN = 1024*1024*40;
    char *pJPEGBuf = new char[JPEG_DATA_LEN];
    char filename[64] = {0};
    char svrAddr[64];

    BOOL bResult = FALSE;
    long deviceID, channel;
            

#if (USE_SOCKET == 1)
    // create socket thread
    pthread_t tid_socket;
    pthread_create(&tid_socket,NULL,socket_thread,NULL);
#endif

#if (USE_MODBUS == 1)
    // create socket thread
    pthread_t tid_modbus;
    pthread_create(&tid_socket,NULL,modbus_thread,NULL);
#endif


    i = 0;

    while(1)
    {
        #if (USE_SOCKET == 1)
      /*  if(socket_cmd)
        {
            if(capture_pic())
                printf("running_count: %d\n\n",running_count++);            
        }*/
        #else
           
            //if(bNeedConnect[i] == 0)
            if(1)
            {
                socket_cmd = 0;
                for(i = 0; i < NUM_CAMARAS; i ++)
                {
                    if(bNeedConnect[i] == 0)
                    {
                        deviceID = arrayLoginID[i];
                        channel = 0;
                    }
                    else
                    {
                        printf("the %d camera Error! Login first!\n",i);
                        continue;
                    }
                    printf("demo deviceID = %d, channel = %d, start capture \n",deviceID, channel);

                    sprintf(filename, "./data/%s.jpg",cameraIP[i]);

                    if (pJPEGBuf)
                    {
                        memset(pJPEGBuf, 0x00, JPEG_DATA_LEN);
                        //if (bResult = NET_SDK_CaptureJPEGFile_V2(deviceID, channel,filename)) // addedd by EHL 20191014
                        if ((bResult = NET_SDK_CaptureJPEGData_V2(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen)))
                        {
                            /*FILE* fp = fopen(filename, "wb");
                            if (fp)
                            {
                                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                                fclose(fp);
                            }
                            printf("save jpg success.\n");*/

                            printf("dwRetJpegLen = %d, capture jpg success!\n",dwRetJpegLen);
                            bCameraStatus[i] = 1;
                            printf("success bNeedConnect[i]: %d\n\n",bNeedConnect[i]);
                        }
                        else
                        {
                            printf("faild to capture jpeg data!\n");
                            printf("[%s] line:%d  NET_SDK_GetLastError:%d \n", __FILE__, __LINE__, NET_SDK_GetLastError());
                            printf("dwRetJpegLen = %d, bResult = %d \n",dwRetJpegLen, bResult);

                            bCameraStatus[i] = 0;

                            ret = NET_SDK_Logout(arrayLoginID[i]);
                            printf("camera 1 logout, ret = %d\n",ret);
                            bNeedConnect[i] = 1;
                            printf("camera %s bNeedConnect\n",cameraIP[i]);


                            sleep(5);
                            return 0;
                        }
                        printf("2 bNeedConnect[i]: %d\n\n",bNeedConnect[i]);
                    }
                }
                printf("running_count: %d\n\n",running_count++);
                printf("bNeedConnect[i]: %d\n\n",bNeedConnect[i]);
                sleep(1);
            }
        #endif
	}

	//m_pDeviceMan->Stop();
	//m_pDeviceMan->Quit();

	NET_SDK_Cleanup();

	return 0;
}

=======
// SDKDemo.cpp : 定义控制台应用程序的入口点。
//

//#include "DeviceManager.h"
#include "DVR_NET_SDK.h"
#include "Typedef.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <cstddef>
#include <sys/stat.h>
#define FILEPATH_MAX (80)
#define LOG_IN_FILE 1

#define SAVE_JPG_ALL 1

#define USE_CAMERA 0

#define USE_SOCKET 1
#if (USE_SOCKET == 1)  
#include <sockserver.h>
#endif


#define USE_MODBUS 1
#if (USE_MODBUS == 1)
#include "modbus.h"
#include "unit-test.h"
#endif


#define LOGIN_OUT_TEST 0

//#include "unit-test.h"

BOOL bCameraStatus[255];
char bNeedConnect[255];
unsigned int arrayLoginID[255];

//#define NUM_CAMARAS 1

#if (NUM_CAMARAS>1)
const char *cameraIP[NUM_CAMARAS] = {"192.168.0.249", "192.168.0.253"};
#else
const char *cameraIP[NUM_CAMARAS] = {"192.168.0.249"}; // 2019.11.11 test
#endif

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

int device_capture(LONG index, const char *arg)
{
    int i, ret = 0;
    char filename[64] = {0};
    char cameraStatus = 0;
    BOOL bResult = FALSE;

    long deviceID = arrayLoginID[index];

    long channel = 0;

    sprintf(filename, "./data/%s.jpg",arg);

#if 0
    // delete the old jpg
    if(access(filename, F_OK) == 0) // if file exist
    {
        if(unlink(filename)<0){
            printf("unlink(%s) error !\n",filename);
        }
        if(access(filename, F_OK) != 0)
        {
            //printf("delete (%s) OK !\n",filename);
        }
    }
#endif


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

        /*    FILE* fp = fopen(filename, "wb");
            if (fp)
            {
                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                fclose(fp);
            }
            cameraStatus = 1;
            printf("save jpg success!\n");*/


#if (SAVE_JPG_ALL == 1)
            time_t nSeconds;
            time(&nSeconds);
            struct tm * pTM;
            pTM = localtime(&nSeconds);
            
            /*if(index == 1)
            {
                sprintf(filename, "./%s/%02d-%02d-%02d.jpg",cameraIP[i],pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
            }
            else
            {
                sprintf(filename, "./253/%02d-%02d-%02d.jpg",pTM->tm_hour, pTM->tm_min, pTM->tm_sec); 
            }*/
            sprintf(filename, "./%s/%02d-%02d-%02d.jpg",cameraIP[index],pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
            


            FILE* fp = fopen(filename, "wb");
            if (fp)
            {
                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                fclose(fp);
            }
            cameraStatus = 1;
            printf("save jpg success!\n");
            //delete[] tempstr;

#endif

            printf("dwRetJpegLen = %d, get jpg success. \n",dwRetJpegLen);
            bCameraStatus[index] = 1;
            cameraStatus = 1;

        }
        else
        {
            printf("[%s] line:%d  NET_SDK_GetLastError:%d \n", __FILE__, __LINE__, NET_SDK_GetLastError());
            printf("dwRetJpegLen = %d, bResult = %d \n",dwRetJpegLen, bResult);
            printf("faild to capture jpeg data!\n");

#if (LOG_IN_FILE == 1)
            time_t nSeconds;
            time(&nSeconds);
            struct tm * pTM;
            pTM = localtime(&nSeconds);

            //char *tempstr = new char[2048];
            printf("save in file test.log !\n");
            memset(pJPEGBuf, 0x00, JPEG_DATA_LEN);
            FILE* pFile = fopen("./test.log", "a");
            if(pFile)
            {
                sprintf(pJPEGBuf, "%04d-%02d-%02d %02d:%02d:%02d device_capture: IP = %s, NET_SDK_GetLastError:%d !\n", \
                        pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday, \
                        pTM->tm_hour, pTM->tm_min, pTM->tm_sec, \
                        cameraIP[index], ret);
                fwrite(pJPEGBuf, sizeof(char), strlen(pJPEGBuf), pFile);

                fclose(pFile);
            }
            //delete[] tempstr;

#endif
            printf("Logout the device[%d].\n",deviceID);
            NET_SDK_Logout(deviceID);
            ret = NET_SDK_GetLastError();
            sprintf(pJPEGBuf, "[%s] line:%d  NET_SDK_GetLastError:%d failed!\n", __FILE__, __LINE__, ret);
            printf("%s ",pJPEGBuf);
           // bNeedConnect[deviceID] = TRUE;

            cameraStatus = 0;
        }

        delete []pJPEGBuf;
        pJPEGBuf = NULL;
    }
    return cameraStatus;
}


int capture_pic(long index)
{
    if(bNeedConnect[index] == 1)
    {
        printf("bNeedConnect=1, exist capture!\n");
    }
    else if(index < NUM_CAMARAS)
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


void * camera_thread(void *arg)
{
    int i = 0;
    int connectTimeout = 100;
	BOOL m_bConnecting = 1;
	unsigned int			arrayDeviceID[255];
    unsigned int			arrayChannel[255];
	char sDVRIP[64];
	unsigned short nPort;
	char sUsername[64];
	char sPassword[64];
	int channelNum;
	NET_SDK_DEVICEINFO deviceInfo;


    while(m_bConnecting)
	{
        NET_SDK_SetConnectTime(connectTimeout, 1);
        for(i = 0; i < NUM_CAMARAS; i ++)
        {
            sprintf(sDVRIP, cameraIP[i]);
            nPort = 9008;
            strcpy(sUsername, "admin");
            strcpy(sPassword, "123456");

            if(bNeedConnect[i])
            {
                //printf("Login the camera[%d] %s ...\n",i,cameraIP[i]);
                LONG lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
                //printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                 //   lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
                printf("lLoginID:%d, deviceID=%d,  deviceName=%s\n",lLoginID, deviceInfo.deviceID, deviceInfo.deviceName);
                if(lLoginID > 0)
                {
                    arrayLoginID[i] = lLoginID;
                    bNeedConnect[i] = 0;
                    printf("bNeedConnect[%d]=%d\n\n",i,bNeedConnect[i]);
                    //bCameraStatus[i] = TRUE;
                }
                else
                {
                    printf("bNeedConnect[%d]=%d, please check the IP: %s ...\n\n",i,bNeedConnect[i],cameraIP[i]);
                    bNeedConnect[i] = 1;
                   // unsigned int ret = NET_SDK_GetLastError();
                   // printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
                }
            }
        }

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

    }
}

int main(int argc, char * argv[])
{
    int i=0;
    int running_count = 0;

	char command;
	DWORD dwErrorCode = 0;

    // initilize
    for(i = 0; i < 255; i++)
    {
        bNeedConnect[i] = 1;
    }
    printf("main(): bNeedConnect[] = %d %d %d ...\n",bNeedConnect[0],bNeedConnect[1],bNeedConnect[2]);

    DIR *dirp;
    dirp = opendir("data");
    if(NULL==dirp)
          mkdir("data",0775);
    closedir(dirp);

#if (SAVE_JPG_ALL == 1)

    for(i = 0; i < 2; i++)
    {
        dirp = opendir(cameraIP[i]);
        if(NULL==dirp)
          mkdir(cameraIP[i], 0775);
        closedir(dirp);
    }
#endif

#if 1
    time_t nSeconds;
    time(&nSeconds);
    struct tm * pTM;
    pTM = localtime(&nSeconds);

    char *tempstr = new char[2048];
    printf("save in file test.log !\n");
    memset(tempstr, 0x00, 2048);
    FILE* pFile = fopen("./test.log", "a");
    if(pFile)
    {
        /* 系统日期和时间,格式: yyyymmddHHMMSS */
        sprintf(tempstr, "%04d-%02d-%02d %02d:%02d:%02d main() start! NUM_CAMARAS=%d\n", \
                pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday, \
                pTM->tm_hour, pTM->tm_min, pTM->tm_sec,NUM_CAMARAS);

        fwrite(tempstr, sizeof(char), strlen(tempstr), pFile);

        fclose(pFile);
    }
    delete[] tempstr;

#endif

    NET_SDK_Init();
	NET_SDK_SetConnectTime();
	NET_SDK_SetReconnect(10000, TRUE);

#if (USE_CAMERA == 1)
    // create socket thread
    pthread_t tid_camera;
    pthread_create(&tid_camera,NULL,camera_thread,NULL);
#endif

    unsigned int ret;
    LONG lLoginID=0;
    char sDVRIP[64];
    unsigned short nPort;
    char sUsername[64];
    char sPassword[64];
    int channelNum;
    NET_SDK_DEVICEINFO deviceInfo;


#if (LOGIN_OUT_TEST == 1)
    while(1)
    {

        int i=0;
        nPort = 9008;
        strcpy(sUsername, "admin");
        strcpy(sPassword, "123456");
        sprintf(sDVRIP, "192.168.0.%d",249);

        printf("now login 249\n");
        lLoginID = 0;
        while(lLoginID <= 0)
        {
            lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
            printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            if(lLoginID > 0)
            {
                arrayLoginID[i] = lLoginID;
                bNeedConnect[i] = FALSE;
                printf("deviceID = %d,  deviceName = %s, localVideoInputNum = %d, videoInputNum = %d, videoOutputNum = %d", \
                deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
            }
            else
            {
                bNeedConnect[i] = TRUE;
                ret = NET_SDK_GetLastError();
                printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
            }
        }
        arrayLoginID[0] = lLoginID;

/*        sprintf(sDVRIP, "192.168.0.%d",253);

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

        sleep(20);
        printf("\n");
        printf("now logout the camera.\n");
        ret = NET_SDK_Logout(arrayLoginID[0]);
        printf("camera 1 logout, ret = %d\n",ret);

     /*   printf("now logout the camera.\n");
        ret = NET_SDK_Logout(2);
        printf("camera 2 logout, ret = %d\n",ret);*/

        printf("waiting ...\n");
       NET_SDK_Cleanup();
        sleep(61);
        NET_SDK_Init();
        NET_SDK_SetConnectTime();
        NET_SDK_SetReconnect(10000, TRUE);

    }
#endif


    //unsigned int ret;
    BOOL m_bConnecting = 1;

    DWORD dwRetJpegLen = 0;
    const int JPEG_DATA_LEN = 1024*1024*40;
    char *pJPEGBuf = new char[JPEG_DATA_LEN];
    char filename[64] = {0};
    char svrAddr[64];

    BOOL bResult = FALSE;
    long deviceID, channel;
            

#if (USE_SOCKET == 1)
    // create socket thread
    pthread_t tid_socket;
    pthread_create(&tid_socket,NULL,socket_thread,NULL);
#endif

#if (USE_MODBUS == 1)
    // create socket thread
    pthread_t tid_modbus;
    pthread_create(&tid_socket,NULL,modbus_thread,NULL);
#endif


    i = 0;

    while(1)
    {
        #if (USE_SOCKET == 1)
      /*  if(socket_cmd)
        {
            if(capture_pic())
                printf("running_count: %d\n\n",running_count++);            
        }*/
        #else
           
            //if(bNeedConnect[i] == 0)
            if(1)
            {
                socket_cmd = 0;
                for(i = 0; i < NUM_CAMARAS; i ++)
                {
                    if(bNeedConnect[i] == 0)
                    {
                        deviceID = arrayLoginID[i];
                        channel = 0;
                    }
                    else
                    {
                        printf("the %d camera Error! Login first!\n",i);
                        continue;
                    }
                    printf("demo deviceID = %d, channel = %d, start capture \n",deviceID, channel);

                    sprintf(filename, "./data/%s.jpg",cameraIP[i]);

                    if (pJPEGBuf)
                    {
                        memset(pJPEGBuf, 0x00, JPEG_DATA_LEN);
                        //if (bResult = NET_SDK_CaptureJPEGFile_V2(deviceID, channel,filename)) // addedd by EHL 20191014
                        if ((bResult = NET_SDK_CaptureJPEGData_V2(deviceID, channel, pJPEGBuf, JPEG_DATA_LEN, &dwRetJpegLen)))
                        {
                            /*FILE* fp = fopen(filename, "wb");
                            if (fp)
                            {
                                fwrite(pJPEGBuf, sizeof(char), dwRetJpegLen, fp);
                                fclose(fp);
                            }
                            printf("save jpg success.\n");*/

                            printf("dwRetJpegLen = %d, capture jpg success!\n",dwRetJpegLen);
                            bCameraStatus[i] = 1;
                            printf("success bNeedConnect[i]: %d\n\n",bNeedConnect[i]);
                        }
                        else
                        {
                            printf("faild to capture jpeg data!\n");
                            printf("[%s] line:%d  NET_SDK_GetLastError:%d \n", __FILE__, __LINE__, NET_SDK_GetLastError());
                            printf("dwRetJpegLen = %d, bResult = %d \n",dwRetJpegLen, bResult);

                            bCameraStatus[i] = 0;

                            ret = NET_SDK_Logout(arrayLoginID[i]);
                            printf("camera 1 logout, ret = %d\n",ret);
                            bNeedConnect[i] = 1;
                            printf("camera %s bNeedConnect\n",cameraIP[i]);


                            sleep(5);
                            return 0;
                        }
                        printf("2 bNeedConnect[i]: %d\n\n",bNeedConnect[i]);
                    }
                }
                printf("running_count: %d\n\n",running_count++);
                printf("bNeedConnect[i]: %d\n\n",bNeedConnect[i]);
                sleep(1);
            }
        #endif
	}

	//m_pDeviceMan->Stop();
	//m_pDeviceMan->Quit();

	NET_SDK_Cleanup();

	return 0;
}

>>>>>>> a6d2214bda1c223f2b5a1caf2eccb7f94d6903ae
