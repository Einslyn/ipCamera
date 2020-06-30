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
#include "camera.h"
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

char running_debug;
int dataSendTotal;
unsigned int ipaddr_int[255] = {0};
unsigned int num_of_persons[255] = {0};


static unsigned int IPtoInt(char *str_ip)
{
    in_addr addr;
    unsigned int int_ip;
    if(inet_aton(str_ip,&addr))
    {
        int_ip = ntohl(addr.s_addr);
    }
    return int_ip;
}
static int FileSize(const char* fname)
{
    struct stat statbuf;
    if(stat(fname,&statbuf)==0)
        return statbuf.st_size;
    return -1;
}

static int get_ini_intvalue(dictionary *dic,const char *keystr)
{
    int command;
    command = iniparser_getlongint(dic,keystr,-1);
    if(command > -1){
        return command;
    }else{
        printf("ini: can't find %s.\n",keystr);
        return -1;
    }
}

static int run_shell_cmd(const char *command, int timout)
{
    char output[100];
    int retry = timout * 1000;// x 1ms
    int result = 0;
    FILE *p = popen(command, "r");
    printf("the python timout value: %d\n",timout);
    retry = timout;
    while(retry--)
    {
        printf("C++ run python JPG remained: %d / %d\n",retry,timout);
        usleep(1000*1000); // 1 second
    }
    if(p != NULL)
    {
        retry = timout * 1000;
        if(running_debug ==1 ) printf("print the popen() output:\n");
        while(fgets(output, sizeof(output), p) != NULL)
        {
            if(running_debug ==1 ) printf("%s",output);
            if(NULL != strstr(output,"Python done."))
            {
                result = 1;
                printf("popen() return: OK.\n");
            }
            else
            {
                if(running_debug ==1 ) printf("searching ...\n");
            }
            usleep(1000);
            if(!retry--)
                break;
        }
        if(running_debug ==1 ) printf("print the popen() output end.\n");
    }
    pclose(p);
 
    // find the "python.ok" file
    if (access("./data/python.ok", F_OK) == 0)
    {
        printf("check if file .data/python.ok exist: YES\n");
        if(!result) result = 0;
    }
    else
    {
        printf("Error: Can't find python.ok\n");
        result = 0;
    }
    return result;
}

int main(int argc, char * argv[])
{
    int retry = 0;
    int sum = 0;
    char modbus_randomtest_enable = 0;
    char SAVE_JPG_ALL=0;

    char *file1 = (char*)malloc(255);
    char *file2 = (char*)malloc(255);

    int TIME_interval = 60; //second
    int SAVE_JPG_HOUR_START = 7; // no more than 24.
    int SAVE_JPG_HOUR_END = 24; // no more than 24.
    const char *temp = "192.168.255.255";
    char *IP_FORMAT = (char*)malloc(32);
    char *MODBUS_TCP_ADDRESS = (char*)malloc(32);

    // read current time
    volatile int current_time = 0;
    time_t nSeconds;
    struct tm * pTM;
    time(&nSeconds);
    pTM = localtime(&nSeconds);
    current_time = 0;

     ///////////////////////////////////// ini parse ////////////////////////
    // load config from ini file
    dictionary *dic;

    if(NULL == (dic = iniparser_load("./setting.ini"))){
        printf("ERROR: open file failed!\n");
        return -1;
    }
    //这只是用于debug.
    //iniparser_dump_ini(dic, stdout);

    running_debug = get_ini_intvalue(dic, "running:DEBUG");
    modbus_randomtest_enable = get_ini_intvalue(dic, "MODBUS:TEST");
    TIME_interval = get_ini_intvalue(dic, "TIME:PERIOD");

    SAVE_JPG_ALL = get_ini_intvalue(dic, "JPG:SAVE_ALL");
    SAVE_JPG_HOUR_START = get_ini_intvalue(dic, "JPG:HOUR_START");
    SAVE_JPG_HOUR_END = get_ini_intvalue(dic, "JPG:HOUR_END");

    // load IP setting
    temp = iniparser_getstring(dic,"IP:FORMAT","192.168.0.");
    if(strlen(temp) < 32)
        strcpy(IP_FORMAT, temp);
    IP_VALID_START = get_ini_intvalue(dic, "IP:VALID_START");
    IP_VALID_END = get_ini_intvalue(dic, "IP:VALID_END");
    NUM_CAMARAS = get_ini_intvalue(dic, "CAMERA:NUM");
    if(NUM_CAMARAS != (IP_VALID_END + 1 - IP_VALID_START))
    {
        printf("Warning: NUM_CAMARAS != IP_VALID_END + 1 - IP_VALID_START\n");
        NUM_CAMARAS = IP_VALID_END + 1 - IP_VALID_START;
        printf("NUM_CAMARAS is %d\n",NUM_CAMARAS);
    }

    temp = iniparser_getstring(dic,"MODBUS:ADDR",NULL);
    if(strlen(temp) < 32)
        strcpy(MODBUS_TCP_ADDRESS, temp);

    //释放dic相关联的内存。该函数无返回值。
    iniparser_freedict(dic);

    printf("DEBUG mode:%s\n",(1==running_debug) ? "YES" : "NO");
    printf("Modbus random test: %s\n",(1==modbus_randomtest_enable) ? "YES" : "NO");
    printf("Save JPGs within hours: %s\n",(1==SAVE_JPG_ALL) ? "YES" : "NO");
    if(1==SAVE_JPG_ALL)
    {
        printf("%02d:00 -> %02d:00\n",SAVE_JPG_HOUR_START,SAVE_JPG_HOUR_END);
    }
    printf("Update period second: %d\n",TIME_interval);
    printf("Camera IP valid: %s%d -> %d, total %d\n",IP_FORMAT,IP_VALID_START, IP_VALID_END,NUM_CAMARAS);
    printf("MODBUS address: %s \n",MODBUS_TCP_ADDRESS);
    if(TIME_interval < NUM_CAMARAS)
            printf("Warnning: Period < NUM_CAMARAS\n");

    if(1 == running_debug)
    {
        printf("In debug mode, test popen() to call python.\n");
        if(0 != run_shell_cmd("sh ./runpy.sh 2>&1", NUM_CAMARAS*4/3))
        {
            usleep(100000);
            printf("C++: get the python result. Test success.\n");
        }
    }
    printf("\nconfirm ? (Wait 5sec / Ctrl+C)\n");
    
    retry = 5; // second
    while(retry--)
    {
        printf("%d\n",retry);
        sleep(1); // 1s
    }
    printf("\n\n");

    //////////////////////////////////// ini parse ///////////////////////////////////////////


    ////////////////////////////////////// zlog //////////////////////////////////////////////
    int rc;
    zlog_category_t *plog;

    rc = zlog_init("zlog.conf");
    if (rc) {
        printf("init failed\n");
        return -1;
    }

    plog = zlog_get_category("my_cat");
    if (!plog) {
        printf("get cat fail\n");
        zlog_fini();
        return -2;
    }

    zlog_debug(plog,"\n\nApplication start!\n\n");
    zlog_info(plog, "IP range: %s%d -- %d\n",IP_FORMAT,IP_VALID_START,IP_VALID_END);
    //////////////////////////////////// zlog end ///////////////////////////////////////////////

    // initilize IP
    for(int j=0; j < 255; j ++)
    {
        bNeedConnect[j] = FALSE;
        ipaddr_int[j] = 0;
        memset(cameraIP[j],'\0',16);
    }

    printf("main: camera IP set:\n");
    for(int j = IP_VALID_START; j < IP_VALID_END+1; j++)
    {
        ipaddr_int[j] = j;
        bNeedConnect[j] = TRUE;
        sprintf(cameraIP[j],"%s%d",IP_FORMAT, j);
    }
    printf("main: bNeedConnect[] = %d %d %d ...\n",bNeedConnect[0],bNeedConnect[1],bNeedConnect[2]);

    if(1 == running_debug)
    {
        for(int j = IP_VALID_START; j < IP_VALID_END+1; j++)
        {
            printf("Camera No.%d\t %s\n", j+1-IP_VALID_START, cameraIP[j]);
        }
        printf("\n\n");
    }

    DIR *dirp;
    dirp = opendir("data");
    if(NULL==dirp)
          mkdir("data",0775);
    closedir(dirp);

    if(SAVE_JPG_ALL == 1)
    {
        for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
        {
            dirp = opendir(cameraIP[j]);
            if(NULL==dirp)
              mkdir(cameraIP[j], 0775);
            closedir(dirp);
        }
    }

    printf("start to make result.csv.\n");
    std::ofstream file("result.csv",std::ios::trunc | std::ios::out | std::ios::binary);
    if (file)
    {
        file << "date,time" << ",";
        for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
        {
            file << cameraIP[j] << "," ;
        }
        file << "\n";
    }
    else
        printf("failed open result.csv.\n");
    file.close();
    printf("OK to make result.csv.\n");

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief NET_SDK_Init
    ///
    NET_SDK_Init();
	NET_SDK_SetConnectTime();
	NET_SDK_SetReconnect(10000, TRUE);

#if (USE_CAMERA == 1)
    // create  thread
    pthread_t tid_camera;
    pthread_create(&tid_camera,NULL,camera_thread,NULL);
#endif

    // wait for seconds for all cameras login
    printf("Wait for seconds %d cameras login ... \n",NUM_CAMARAS);
    retry = NUM_CAMARAS * 1000; // second
    while(retry--)
    {
        usleep(1000); // 1ms
        if(NUM_CAMARAS == camera_online(IP_VALID_START,IP_VALID_END)) 
            break;
    }
    printf("camera login count: %d \n",camera_online(IP_VALID_START,IP_VALID_END));

#if (USE_MODBUS == 1)
    // create  thread
    pthread_t tid_modbus;
    pthread_create(&tid_modbus,NULL,modbus_thread,MODBUS_TCP_ADDRESS);
    usleep(10000);
#endif

    // 2020-04-30 added
    if(!modbus_randomtest_enable)
        dataSendTotal = NUM_CAMARAS;
    else
        dataSendTotal = 0xFF;

    while(1)
    {
        if(0xFF == dataSendTotal)
        {
            current_time = -1;
            if(1 == running_debug) printf("dataSendTotal = %d, Modbus random test...\n",dataSendTotal);
        }
        
        // if current time == last_time + TIME_interval, update and do something
        if((current_time%TIME_interval) == 0)
        {
            printf("To delete python.ok and result.txt before capture...\n");
            if (access("./data/python.ok", F_OK) == 0)
            {
                remove("./data/python.ok");
                printf("./data/python.ok  removed.\n");
            }
            else
            {
                zlog_debug(plog,"ERROR .data/python.ok not found!");
            }
            if (access("./data/result.txt", F_OK) == 0)
            {
                remove("./data/result.txt");
                printf("./data/result.txt  removed.\n");
            }
            else
            {
                zlog_debug(plog,"ERROR data/result.txt not found!");
            }
            
            printf("\n");

            time(&nSeconds);
            pTM = localtime(&nSeconds);
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);

            // capture jpg
            printf("Start capture jpg\n");
            sum = 0;
            for(int j = IP_VALID_START; j < IP_VALID_END+1; j ++)
            {
                sum += capture_pic(j);
            }
            printf("OK to capture %d pictures \n\n",sum);

            time(&nSeconds);
            pTM = localtime(&nSeconds);
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);

            // run the python program
            printf("Run the python:\n\n");

            if(0 != run_shell_cmd("sh ./runpy.sh", NUM_CAMARAS*4/3))
            {
                usleep(100000);
                printf("C++: the python done.\n");
            }
            else
            {
                zlog_debug(plog,"run run_shell_cmd(*) return 0 ERROR!");
            }
            
            time(&nSeconds);
            pTM = localtime(&nSeconds);
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);

            printf("\n");

            // update the number of people for each IP
            printf("To get the result ...\n\n");

            // open the file of the python output <result.txt>
            std::ifstream in;
            in.open("./data/result.txt", std::ios::in | std::ios::binary);
            if (in.is_open())
            {
                while (!in.eof())
                {
                    char sDVRIP[64];
                    char *ptr = (char*)malloc(255);
                    char ccc = ':';
                    int position = 0;
                    int index_of_IP = 0;
                    char *string = (char*)malloc(255);

                    in.getline(string,100);
                    if(strlen(string) < 4)
                        continue;

                    ptr = strchr(string, ccc); //ptr: point to the string at ':'
                    position = ptr - string;
                    if (ptr)
                    {
                        //printf("The character '%c' is at position: %d  new ptr = %s\n", ccc, position, ptr);

                        // the IP address
                        strcpy(sDVRIP,string);
                        *(sDVRIP + position) = '\0';
                        if(1 == running_debug)
                            printf("IP address = %s\n", sDVRIP);

                        // get the index of the IP address
                        position = strspn(sDVRIP, IP_FORMAT);
                        int temp = 0;
                        int pos = 0;
                        for( pos = 0; pos < 16; pos++)
                        {
                            if('.' == sDVRIP[pos])
                            {
                                temp++;
                                if(3 == temp)
                                    break;
                            }
                        }
                        position = pos+1;
                        index_of_IP = atoi(sDVRIP + position);
                        if((index_of_IP >= IP_VALID_START) && (index_of_IP <= IP_VALID_END))
                        {
                            if(1 == running_debug)
                            printf("IP address index = %d Valid.\n", index_of_IP);
                        }
                        else
                        {
                            printf("IP address index = %d Invalid --------------------------------------------\n", index_of_IP);
                        }

                        // deparse and update the numbers of people
                        //position = strspn(sDVRIP, IP_FORMAT);// point to '.*:*.jpg',
                        string = strchr(ptr, '.'); // ptr: point to ':*.jpg', string: point to '.jpg'
                        if(string)
                        {
                            position = string - ptr;
                            *(ptr + position) = '\0';
                            ptr++;
                            num_of_persons[index_of_IP] = atoi(ptr); // convert a string to an integer
                            if(1 == running_debug)
                                printf("number of people = %d\n", num_of_persons[index_of_IP]);
                        }
                        else
                        {
                            printf("ERROR str = %s (expect '.') ------------------------------------------------\n", ptr);
                        }
                    }
                    else
                    {
                       printf("%s \nERROR line (expect ':') ----------------------------------------------------\n",string);
                    }
                } // end of while (!in.eof())
            } // end of result.txt
            else
            {
                zlog_debug(plog,"result.txt not found. Python program error. Reset all register in modbus.");
                for(int j=0; j < 255; j ++)
                {
                    num_of_persons[j] = 0;
                }
                printf("\n");
            }
            in.close();

            // update the numbers.
            for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
            {
                if(TRUE == bNeedConnect[j])
                {
                    num_of_persons[j] = 0;
                    zlog_debug(plog,"Reset the number of persons in classroom. Offlined camera IP index = [%d].\n",j);
                }
            }
            printf("main thread  [IP] and number of persons:\n");
            for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
            {
                printf("[%d]%04d\t",j,num_of_persons[j]);
            }
            printf("\n");
            printf("Update the numbers ... End.\n\n");


            // deal with the data
            if(1 == SAVE_JPG_ALL)
            {
                for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                {
                    sprintf(file1, "./data/%s.jpg",cameraIP[j]);
                    if (access(file1, F_OK) == 0)
                    {
                        remove(file1);
                    }
                    else
                        printf("%s not found.\n",file1);
                    sprintf(file1, "./data/%s:%d.jpg",cameraIP[j],num_of_persons[j]);
                    if (access(file1, F_OK) == 0)
                    {
                        sprintf(file2, "./%s/%02d-%02d:%d.jpg",cameraIP[j],pTM->tm_hour,pTM->tm_min,num_of_persons[j]);
                        rename(file1,file2);
                    }
                    else
                        printf("%s not found.\n",file1);
                }
                printf("OK to delete original jpg and save result jpg.\n\n");
            }
            else
            {
                system("rm ./data/*.jpg");
            }

            printf("Start to save numbers in result.csv...\n");
            std::ofstream file("result.csv",std::ios::app);
            if (file)
            {
                file << pTM->tm_year+1900 << "-" << pTM->tm_mon+1 << "-" << pTM->tm_mday<<","<<pTM->tm_hour<<":"<<pTM->tm_min<<":"<<pTM->tm_sec<<",";
                for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                {
                    file << num_of_persons[j] << "," ;
                }
                file << "\n";
                printf("OK to save numbers in result.csv \n\n");
            }
            else
                printf("ERROR open result.csv \n\n");
            file.close();

            printf("%d current time %02d:%02d:%02d \n",SAVE_JPG_ALL,pTM->tm_hour,pTM->tm_min,pTM->tm_sec);
            printf("done\n");

            // deal with the file system.
            if((SAVE_JPG_HOUR_START == pTM->tm_hour) && (0 == SAVE_JPG_ALL))
            {
                printf("save time start, SAVE_JPG_ALL set 1\n");
                SAVE_JPG_ALL = 1;
                #if (LOG_IN_FILE == 1)
                plog = zlog_get_category("my_cat");
                if (!plog) {
                    printf("get cat fail\n");
                    zlog_fini();
                }
                else
                {
                    zlog_info(plog,"SAVE_JPG_ALL=1, start save JPGs of one day--------------------\n");
                }
                #endif
            }
            else if((SAVE_JPG_HOUR_END == pTM->tm_hour) && (1 == SAVE_JPG_ALL))
            {
                printf("save time end, SAVE_JPG_ALL reset 0\n");
                SAVE_JPG_ALL = 0;
                for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                {
                    sprintf(file2, "rm ./%s/*.jpg",cameraIP[j]);
                    system(file2);
                }
                // avoid jpg run out of the disk
                #if (LOG_IN_FILE == 1)
                plog = zlog_get_category("my_cat");
                if (!plog) {
                    printf("get cat fail\n");
                    zlog_fini();
                }
                else
                {
                    zlog_info(plog,"SAVE_JPG_ALL=0, clean all JPGs of one day----------------\n\n\n\n");
                }
                #endif

                // avoid the csv file run out of the disk.
                if(FileSize("result.csv") > 10*1024*1024) // 10MB
                {
                    printf("the result.csv over size, delete----------------\n");
                    std::ofstream file("result.csv",std::ios::trunc | std::ios::out);  // when open the file, delete it first if exist.
                    if (file)
                    {
                        file << "date,time" <<",";
                        for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                        {
                            file << cameraIP[j] << "," ;
                        }
                        file << "\n";
                    }
                    else
                        printf("failed open result.csv.\n");
                    file.close();
                }
            }


        } // end of if (one period flag)

        // read current time
        time(&nSeconds);
        pTM = localtime(&nSeconds);
        current_time = ((int)pTM->tm_min) * 60 + (int)pTM->tm_sec;


        if(1 == running_debug)
        {
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);
        }

        usleep(1000000);
	}

	//m_pDeviceMan->Stop();
	//m_pDeviceMan->Quit();

	NET_SDK_Cleanup();

    zlog_fini();

	return 0;
}
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
#include "camera.h"
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

char running_debug;
int dataSendTotal;
unsigned int ipaddr_int[255] = {0};
unsigned int num_of_persons[255] = {0};


static unsigned int IPtoInt(char *str_ip)
{
    in_addr addr;
    unsigned int int_ip;
    if(inet_aton(str_ip,&addr))
    {
        int_ip = ntohl(addr.s_addr);
    }
    return int_ip;
}
static int FileSize(const char* fname)
{
    struct stat statbuf;
    if(stat(fname,&statbuf)==0)
        return statbuf.st_size;
    return -1;
}

static int get_ini_intvalue(dictionary *dic,const char *keystr)
{
    int command;
    command = iniparser_getlongint(dic,keystr,-1);
    if(command > -1){
        return command;
    }else{
        printf("ini: can't find %s.\n",keystr);
        return -1;
    }
}

static int run_shell_cmd(const char *command, int timout)
{
    char output[100];
    int retry = timout * 1000;// x 1ms
    int result = 0;
    FILE *p = popen(command, "r");
    printf("the python timout value: %d\n",timout);
    retry = timout;
    while(retry--)
    {
        printf("C++ run python JPG remained: %d / %d\n",retry,timout);
        usleep(1000*1000); // 1 second
    }
    if(p != NULL)
    {
        retry = timout * 1000;
        if(running_debug ==1 ) printf("print the popen() output:\n");
        while(fgets(output, sizeof(output), p) != NULL)
        {
            if(running_debug ==1 ) printf("%s",output);
            if(NULL != strstr(output,"Python done."))
            {
                result = 1;
                printf("popen() return: OK.\n");
            }
            else
            {
                if(running_debug ==1 ) printf("searching ...\n");
            }
            usleep(1000);
            if(!retry--)
                break;
        }
        if(running_debug ==1 ) printf("print the popen() output end.\n");
    }
    pclose(p);
 
    // find the "python.ok" file
    if (access("./data/python.ok", F_OK) == 0)
    {
        printf("check if file .data/python.ok exist: YES\n");
        if(!result) result = 0;
    }
    else
    {
        printf("Error: Can't find python.ok\n");
        result = 0;
    }
    return result;
}

int main(int argc, char * argv[])
{
    int retry = 0;
    int sum = 0;
    char modbus_randomtest_enable = 0;
    char SAVE_JPG_ALL=0;

    char *file1 = (char*)malloc(255);
    char *file2 = (char*)malloc(255);

    int TIME_interval = 60; //second
    int SAVE_JPG_HOUR_START = 7; // no more than 24.
    int SAVE_JPG_HOUR_END = 24; // no more than 24.
    const char *temp = "192.168.255.255";
    char *IP_FORMAT = (char*)malloc(32);
    char *MODBUS_TCP_ADDRESS = (char*)malloc(32);

    // read current time
    volatile int current_time = 0;
    time_t nSeconds;
    struct tm * pTM;
    time(&nSeconds);
    pTM = localtime(&nSeconds);
    current_time = 0;

     ///////////////////////////////////// ini parse ////////////////////////
    // load config from ini file
    dictionary *dic;

    if(NULL == (dic = iniparser_load("./setting.ini"))){
        printf("ERROR: open file failed!\n");
        return -1;
    }
    //这只是用于debug.
    //iniparser_dump_ini(dic, stdout);

    running_debug = get_ini_intvalue(dic, "running:DEBUG");
    modbus_randomtest_enable = get_ini_intvalue(dic, "MODBUS:TEST");
    TIME_interval = get_ini_intvalue(dic, "TIME:PERIOD");

    SAVE_JPG_ALL = get_ini_intvalue(dic, "JPG:SAVE_ALL");
    SAVE_JPG_HOUR_START = get_ini_intvalue(dic, "JPG:HOUR_START");
    SAVE_JPG_HOUR_END = get_ini_intvalue(dic, "JPG:HOUR_END");

    // load IP setting
    temp = iniparser_getstring(dic,"IP:FORMAT","192.168.0.");
    if(strlen(temp) < 32)
        strcpy(IP_FORMAT, temp);
    IP_VALID_START = get_ini_intvalue(dic, "IP:VALID_START");
    IP_VALID_END = get_ini_intvalue(dic, "IP:VALID_END");
    NUM_CAMARAS = get_ini_intvalue(dic, "CAMERA:NUM");
    if(NUM_CAMARAS != (IP_VALID_END + 1 - IP_VALID_START))
    {
        printf("Warning: NUM_CAMARAS != IP_VALID_END + 1 - IP_VALID_START\n");
        NUM_CAMARAS = IP_VALID_END + 1 - IP_VALID_START;
        printf("NUM_CAMARAS is %d\n",NUM_CAMARAS);
    }

    temp = iniparser_getstring(dic,"MODBUS:ADDR",NULL);
    if(strlen(temp) < 32)
        strcpy(MODBUS_TCP_ADDRESS, temp);

    //释放dic相关联的内存。该函数无返回值。
    iniparser_freedict(dic);

    printf("DEBUG mode:%s\n",(1==running_debug) ? "YES" : "NO");
    printf("Modbus random test: %s\n",(1==modbus_randomtest_enable) ? "YES" : "NO");
    printf("Save JPGs within hours: %s\n",(1==SAVE_JPG_ALL) ? "YES" : "NO");
    if(1==SAVE_JPG_ALL)
    {
        printf("%02d:00 -> %02d:00\n",SAVE_JPG_HOUR_START,SAVE_JPG_HOUR_END);
    }
    printf("Update period second: %d\n",TIME_interval);
    printf("Camera IP valid: %s%d -> %d, total %d\n",IP_FORMAT,IP_VALID_START, IP_VALID_END,NUM_CAMARAS);
    printf("MODBUS address: %s \n",MODBUS_TCP_ADDRESS);
    if(TIME_interval < NUM_CAMARAS)
            printf("Warnning: Period < NUM_CAMARAS\n");

    if(1 == running_debug)
    {
        printf("In debug mode, test popen() to call python.\n");
        if(0 != run_shell_cmd("sh ./runpy.sh 2>&1", NUM_CAMARAS*4/3))
        {
            usleep(100000);
            printf("C++: get the python result. Test success.\n");
        }
    }
    printf("\nconfirm ? (Wait 5sec / Ctrl+C)\n");
    
    retry = 5; // second
    while(retry--)
    {
        printf("%d\n",retry);
        sleep(1); // 1s
    }
    printf("\n\n");

    //////////////////////////////////// ini parse ///////////////////////////////////////////


    ////////////////////////////////////// zlog //////////////////////////////////////////////
    int rc;
    zlog_category_t *plog;

    rc = zlog_init("zlog.conf");
    if (rc) {
        printf("init failed\n");
        return -1;
    }

    plog = zlog_get_category("my_cat");
    if (!plog) {
        printf("get cat fail\n");
        zlog_fini();
        return -2;
    }

    zlog_debug(plog,"\n\nApplication start!\n\n");
    zlog_info(plog, "IP range: %s%d -- %d\n",IP_FORMAT,IP_VALID_START,IP_VALID_END);
    //////////////////////////////////// zlog end ///////////////////////////////////////////////

    // initilize IP
    for(int j=0; j < 255; j ++)
    {
        bNeedConnect[j] = FALSE;
        ipaddr_int[j] = 0;
        memset(cameraIP[j],'\0',16);
    }

    printf("main: camera IP set:\n");
    for(int j = IP_VALID_START; j < IP_VALID_END+1; j++)
    {
        ipaddr_int[j] = j;
        bNeedConnect[j] = TRUE;
        sprintf(cameraIP[j],"%s%d",IP_FORMAT, j);
    }
    printf("main: bNeedConnect[] = %d %d %d ...\n",bNeedConnect[0],bNeedConnect[1],bNeedConnect[2]);

    if(1 == running_debug)
    {
        for(int j = IP_VALID_START; j < IP_VALID_END+1; j++)
        {
            printf("Camera No.%d\t %s\n", j+1-IP_VALID_START, cameraIP[j]);
        }
        printf("\n\n");
    }

    DIR *dirp;
    dirp = opendir("data");
    if(NULL==dirp)
          mkdir("data",0775);
    closedir(dirp);

    if(SAVE_JPG_ALL == 1)
    {
        for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
        {
            dirp = opendir(cameraIP[j]);
            if(NULL==dirp)
              mkdir(cameraIP[j], 0775);
            closedir(dirp);
        }
    }

    printf("start to make result.csv.\n");
    std::ofstream file("result.csv",std::ios::trunc | std::ios::out | std::ios::binary);
    if (file)
    {
        file << "date,time" << ",";
        for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
        {
            file << cameraIP[j] << "," ;
        }
        file << "\n";
    }
    else
        printf("failed open result.csv.\n");
    file.close();
    printf("OK to make result.csv.\n");

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief NET_SDK_Init
    ///
    NET_SDK_Init();
	NET_SDK_SetConnectTime();
	NET_SDK_SetReconnect(10000, TRUE);

#if (USE_CAMERA == 1)
    // create  thread
    pthread_t tid_camera;
    pthread_create(&tid_camera,NULL,camera_thread,NULL);
#endif

    // wait for seconds for all cameras login
    printf("Wait for seconds %d cameras login ... \n",NUM_CAMARAS);
    retry = NUM_CAMARAS * 1000; // second
    while(retry--)
    {
        usleep(1000); // 1ms
        if(NUM_CAMARAS == camera_online(IP_VALID_START,IP_VALID_END)) 
            break;
    }
    printf("camera login count: %d \n",camera_online(IP_VALID_START,IP_VALID_END));

#if (USE_MODBUS == 1)
    // create  thread
    pthread_t tid_modbus;
    pthread_create(&tid_modbus,NULL,modbus_thread,MODBUS_TCP_ADDRESS);
    usleep(10000);
#endif

    // 2020-04-30 added
    if(!modbus_randomtest_enable)
        dataSendTotal = NUM_CAMARAS;
    else
        dataSendTotal = 0xFF;

    while(1)
    {
        if(0xFF == dataSendTotal)
        {
            current_time = -1;
            if(1 == running_debug) printf("dataSendTotal = %d, Modbus random test...\n",dataSendTotal);
        }
        
        // if current time == last_time + TIME_interval, update and do something
        if((current_time%TIME_interval) == 0)
        {
            printf("To delete python.ok and result.txt before capture...\n");
            if (access("./data/python.ok", F_OK) == 0)
            {
                remove("./data/python.ok");
                printf("./data/python.ok  removed.\n");
            }
            else
            {
                zlog_debug(plog,"ERROR .data/python.ok not found!");
            }
            if (access("./data/result.txt", F_OK) == 0)
            {
                remove("./data/result.txt");
                printf("./data/result.txt  removed.\n");
            }
            else
            {
                zlog_debug(plog,"ERROR data/result.txt not found!");
            }
            
            printf("\n");

            time(&nSeconds);
            pTM = localtime(&nSeconds);
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);

            // capture jpg
            printf("Start capture jpg\n");
            sum = 0;
            for(int j = IP_VALID_START; j < IP_VALID_END+1; j ++)
            {
                sum += capture_pic(j);
            }
            printf("OK to capture %d pictures \n\n",sum);

            time(&nSeconds);
            pTM = localtime(&nSeconds);
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);

            // run the python program
            printf("Run the python:\n\n");

            if(0 != run_shell_cmd("sh ./runpy.sh", NUM_CAMARAS*4/3))
            {
                usleep(100000);
                printf("C++: the python done.\n");
            }
            else
            {
                zlog_debug(plog,"run run_shell_cmd(*) return 0 ERROR!");
            }
            
            time(&nSeconds);
            pTM = localtime(&nSeconds);
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);

            printf("\n");

            // update the number of people for each IP
            printf("To get the result ...\n\n");

            // open the file of the python output <result.txt>
            std::ifstream in;
            in.open("./data/result.txt", std::ios::in | std::ios::binary);
            if (in.is_open())
            {
                while (!in.eof())
                {
                    char sDVRIP[64];
                    char *ptr = (char*)malloc(255);
                    char ccc = ':';
                    int position = 0;
                    int index_of_IP = 0;
                    char *string = (char*)malloc(255);

                    in.getline(string,100);
                    if(strlen(string) < 4)
                        continue;

                    ptr = strchr(string, ccc); //ptr: point to the string at ':'
                    position = ptr - string;
                    if (ptr)
                    {
                        //printf("The character '%c' is at position: %d  new ptr = %s\n", ccc, position, ptr);

                        // the IP address
                        strcpy(sDVRIP,string);
                        *(sDVRIP + position) = '\0';
                        if(1 == running_debug)
                            printf("IP address = %s\n", sDVRIP);

                        // get the index of the IP address
                        position = strspn(sDVRIP, IP_FORMAT);
                        int temp = 0;
                        int pos = 0;
                        for( pos = 0; pos < 16; pos++)
                        {
                            if('.' == sDVRIP[pos])
                            {
                                temp++;
                                if(3 == temp)
                                    break;
                            }
                        }
                        position = pos+1;
                        index_of_IP = atoi(sDVRIP + position);
                        if((index_of_IP >= IP_VALID_START) && (index_of_IP <= IP_VALID_END))
                        {
                            if(1 == running_debug)
                            printf("IP address index = %d Valid.\n", index_of_IP);
                        }
                        else
                        {
                            printf("IP address index = %d Invalid --------------------------------------------\n", index_of_IP);
                        }

                        // deparse and update the numbers of people
                        //position = strspn(sDVRIP, IP_FORMAT);// point to '.*:*.jpg',
                        string = strchr(ptr, '.'); // ptr: point to ':*.jpg', string: point to '.jpg'
                        if(string)
                        {
                            position = string - ptr;
                            *(ptr + position) = '\0';
                            ptr++;
                            num_of_persons[index_of_IP] = atoi(ptr); // convert a string to an integer
                            if(1 == running_debug)
                                printf("number of people = %d\n", num_of_persons[index_of_IP]);
                        }
                        else
                        {
                            printf("ERROR str = %s (expect '.') ------------------------------------------------\n", ptr);
                        }
                    }
                    else
                    {
                       printf("%s \nERROR line (expect ':') ----------------------------------------------------\n",string);
                    }
                } // end of while (!in.eof())
            } // end of result.txt
            else
            {
                zlog_debug(plog,"result.txt not found. Python program error. Reset all register in modbus.");
                for(int j=0; j < 255; j ++)
                {
                    num_of_persons[j] = 0;
                }
                printf("\n");
            }
            in.close();

            // update the numbers.
            for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
            {
                if(TRUE == bNeedConnect[j])
                {
                    num_of_persons[j] = 0;
                    zlog_debug(plog,"Reset the number of persons in classroom. Offlined camera IP index = [%d].\n",j);
                }
            }
            printf("main thread  [IP] and number of persons:\n");
            for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
            {
                printf("[%d]%04d\t",j,num_of_persons[j]);
            }
            printf("\n");
            printf("Update the numbers ... End.\n\n");


            // deal with the data
            if(1 == SAVE_JPG_ALL)
            {
                for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                {
                    sprintf(file1, "./data/%s.jpg",cameraIP[j]);
                    if (access(file1, F_OK) == 0)
                    {
                        remove(file1);
                    }
                    else
                        printf("%s not found.\n",file1);
                    sprintf(file1, "./data/%s:%d.jpg",cameraIP[j],num_of_persons[j]);
                    if (access(file1, F_OK) == 0)
                    {
                        sprintf(file2, "./%s/%02d-%02d:%d.jpg",cameraIP[j],pTM->tm_hour,pTM->tm_min,num_of_persons[j]);
                        rename(file1,file2);
                    }
                    else
                        printf("%s not found.\n",file1);
                }
                printf("OK to delete original jpg and save result jpg.\n\n");
            }
            else
            {
                system("rm ./data/*.jpg");
            }

            printf("Start to save numbers in result.csv...\n");
            std::ofstream file("result.csv",std::ios::app);
            if (file)
            {
                file << pTM->tm_year+1900 << "-" << pTM->tm_mon+1 << "-" << pTM->tm_mday<<","<<pTM->tm_hour<<":"<<pTM->tm_min<<":"<<pTM->tm_sec<<",";
                for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                {
                    file << num_of_persons[j] << "," ;
                }
                file << "\n";
                printf("OK to save numbers in result.csv \n\n");
            }
            else
                printf("ERROR open result.csv \n\n");
            file.close();

            printf("%d current time %02d:%02d:%02d \n",SAVE_JPG_ALL,pTM->tm_hour,pTM->tm_min,pTM->tm_sec);
            printf("done\n");

            // deal with the file system.
            if((SAVE_JPG_HOUR_START == pTM->tm_hour) && (0 == SAVE_JPG_ALL))
            {
                printf("save time start, SAVE_JPG_ALL set 1\n");
                SAVE_JPG_ALL = 1;
                #if (LOG_IN_FILE == 1)
                plog = zlog_get_category("my_cat");
                if (!plog) {
                    printf("get cat fail\n");
                    zlog_fini();
                }
                else
                {
                    zlog_info(plog,"SAVE_JPG_ALL=1, start save JPGs of one day--------------------\n");
                }
                #endif
            }
            else if((SAVE_JPG_HOUR_END == pTM->tm_hour) && (1 == SAVE_JPG_ALL))
            {
                printf("save time end, SAVE_JPG_ALL reset 0\n");
                SAVE_JPG_ALL = 0;
                for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                {
                    sprintf(file2, "rm ./%s/*.jpg",cameraIP[j]);
                    system(file2);
                }
                // avoid jpg run out of the disk
                #if (LOG_IN_FILE == 1)
                plog = zlog_get_category("my_cat");
                if (!plog) {
                    printf("get cat fail\n");
                    zlog_fini();
                }
                else
                {
                    zlog_info(plog,"SAVE_JPG_ALL=0, clean all JPGs of one day----------------\n\n\n\n");
                }
                #endif

                // avoid the csv file run out of the disk.
                if(FileSize("result.csv") > 10*1024*1024) // 10MB
                {
                    printf("the result.csv over size, delete----------------\n");
                    std::ofstream file("result.csv",std::ios::trunc | std::ios::out);  // when open the file, delete it first if exist.
                    if (file)
                    {
                        file << "date,time" <<",";
                        for(int j=IP_VALID_START; j < IP_VALID_END+1; j ++)
                        {
                            file << cameraIP[j] << "," ;
                        }
                        file << "\n";
                    }
                    else
                        printf("failed open result.csv.\n");
                    file.close();
                }
            }


        } // end of if (one period flag)

        // read current time
        time(&nSeconds);
        pTM = localtime(&nSeconds);
        current_time = ((int)pTM->tm_min) * 60 + (int)pTM->tm_sec;


        if(1 == running_debug)
        {
            printf("current time %02d:%02d:%02d \n",pTM->tm_hour,pTM->tm_min,pTM->tm_sec);
        }

        usleep(1000000);
	}

	//m_pDeviceMan->Stop();
	//m_pDeviceMan->Quit();

	NET_SDK_Cleanup();

    zlog_fini();

	return 0;
}
>>>>>>> a6d2214bda1c223f2b5a1caf2eccb7f94d6903ae
