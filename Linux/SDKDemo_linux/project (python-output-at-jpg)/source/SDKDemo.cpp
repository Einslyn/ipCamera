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

#include <sockserver.h>
#include "time.h"

#define FILEPATH_MAX (80)
#define LOG_IN_FILE 1

#define SAVE_JPG_ALL 1

#define USE_CAMERA 1

#define USE_SOCKET 0



#define USE_MODBUS 1
#if (USE_MODBUS == 1)
#include "modbus.h"
#include "unit-test.h"
#endif


#define LOGIN_OUT_TEST 0

//#include "unit-test.h

const char *IP_FORMAT =  (char*)"192.168.0."; // cannot be changed

BOOL bCameraStatus[255];
char bNeedConnect[255];
unsigned int arrayLoginID[255];

char cameraIP[NUM_CAMARAS][16] = {0};

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

    time_t nSeconds;
    struct tm * pTM;

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

            time(&nSeconds);

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
            time(&nSeconds);
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
        for(i = IP_VALID_START; i < IP_VALID_END+1; i ++)
        {
            strcpy(sDVRIP, cameraIP[i]);
            nPort = 9008;
            strcpy(sUsername, "admin");
            strcpy(sPassword, "123456");

            if(bNeedConnect[i])
            {
                printf("camera_thread : Login the camera[%d] %s ...\n",i,cameraIP[i]);

                LONG lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
                /*
                printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
                    lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
                */
                printf("lLoginID:%d, deviceID=%d,  deviceName=%s\n\n",lLoginID, deviceInfo.deviceID, deviceInfo.deviceName);

                if(lLoginID > 0)
                {
                    arrayLoginID[i] = lLoginID;
                    bNeedConnect[i] = 0;
                    bCameraStatus[i] = TRUE;
                    printf("bNeedConnect[%d]=%d,  Camera status OK.\n\n",i,bNeedConnect[i]);
                }
                else
                {
                    //printf("bNeedConnect[%d]=%d ...\n\n",i,bNeedConnect[i]);
                    bNeedConnect[i] = 1;
                    bCameraStatus[i] = 0;
                   // unsigned int ret = NET_SDK_GetLastError();
                   // printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);
                }

                // print the number of cameras exist.
                int sum = 0;
                for(int j = IP_VALID_START; j < IP_VALID_END+1; j ++)
                {
                    sum += bNeedConnect[j];
                }
                printf("%d of %d cameras exist.\n\n",IP_VALID_END-IP_VALID_START - sum + 1, IP_VALID_END-IP_VALID_START + 1);


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
        bNeedConnect[i] = 0;
        memset(cameraIP[i],'\0',16);
    }

    printf("main: camera IP set:\n");
    for(i = IP_VALID_START; i < IP_VALID_END+1; i++)
    {
        bNeedConnect[i] = 1;
        sprintf(cameraIP[i],"%s%d\0",IP_FORMAT, i);
    }
    printf("main: bNeedConnect[] = %d %d %d ...\n",bNeedConnect[0],bNeedConnect[1],bNeedConnect[2]);

    for(i = IP_VALID_START; i < IP_VALID_END+1; i++)
    {
        printf("Camera No.%d\t %s\n", i+1-IP_VALID_START, cameraIP[i]);
    }
    printf("\n\n");

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
        sprintf(tempstr, "%04d-%02d-%02d %02d:%02d:%02d main start! NUM_CAMARAS=%d\n", \
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
    pthread_create(&tid_modbus,NULL,modbus_thread,NULL);
#endif

    i = 0;

    // 2020-04-30 added
    dataSendTotal = 0xFF; // means the JAVA socket client don't connect.

    const int TIME_interval = 60; //second
    volatile int current_time = 0;

    // read current time
    time(&nSeconds);
    pTM = localtime(&nSeconds);
    current_time = 0;

    while(1)
    {
        // if current time == last_time + TIME_interval, update and do something
        if((current_time%TIME_interval) == 0)
        {
            // run the python program
            printf("Run the python:\n\n");
            system("python ~/Program/LCFCN/run.py");
        }


        // get the feedback from python (CMD terminal).
        //
        if(((current_time)%TIME_interval) == (TIME_interval-1))
        {
            // update the number of people for each IP
            printf("Update the numbers ...\n\n");
            DIR * dp;
            struct dirent *filename;
            dp = opendir("data");
            if (!dp)
            {
            fprintf(stderr,"open directory error\n");
            return 0;
            }
            while (filename=readdir(dp))
            {
                printf("filename:%-10s\td_info:%ld\t d_reclen:%us\n",
                filename->d_name,filename->d_ino,filename->d_reclen);

                char *ptr = (char*)malloc(255);
                int position = 0;
                int index_of_IP = 0;
                char *string = (char*)malloc(255);
                char ccc = ':';

                string = (char*)filename->d_name;
                ptr = strchr(string, ccc);
                position = ptr - string;

                if (ptr)
                {
                    //printf("The character '%c' is at position: %d  new ptr = %s\n", ccc, position, ptr);

                    // the IP address
                    strcpy(sDVRIP,string);
                    *(sDVRIP + position) = '\0';
                    printf("IP address = %s\n", sDVRIP);

                    // get the index of the IP address
                    position = strspn(sDVRIP, IP_FORMAT);
                    int j = 0;
                    for( i = 0; i < 16; i++)
                    {
                        if('.' == sDVRIP[i])
                        {
                            j++;
                            if(3 == j)
                                break;
                        }
                    }
                    position = i+1;
                    index_of_IP = atoi(sDVRIP + position);
                    if((index_of_IP>IP_VALID_START) && (index_of_IP<IP_VALID_END))
                    {
                        printf("IP address index = %d Valid.................\n", index_of_IP);

                        // update the ipaddr_int[] and num_of_persons[]

                    }
                    else
                    {
                        printf("IP address index = %d Invalid\n", index_of_IP);
                    }


                    // deparse the numbers
                    position = strspn(sDVRIP, IP_FORMAT);
                    string = strchr(ptr, '.');
                    position = string - ptr;
                    *(ptr + position) = '\0';
                    ptr++;
                    printf("number of people = %d\n", atoi(ptr));

                }
                else
                {
                   printf("The character was not found\n");
                }

                printf("\n");
            }
            closedir(dp);

            //delete the result jpg files

            printf("Update the numbers ... End.\n\n");
        }



        // read current time
        time(&nSeconds);
        pTM = localtime(&nSeconds);
        current_time = ((int)pTM->tm_min) * 60 + (int)pTM->tm_sec;

        printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);

        usleep(1000000);
	}

	//m_pDeviceMan->Stop();
	//m_pDeviceMan->Quit();

	NET_SDK_Cleanup();

	return 0;
}
