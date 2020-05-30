
#ifndef _SOCKSERVER_H_
#define _SOCKSERVER_H_

#define MAX_BYTES   2048
#define SOCKET_PORT 8888

#define NUM_CAMARAS   2 //2019.11.11
#define NUM_CAMARAS_MAX   100 //2019.11.11

#include "Typedef.h"

extern char dataSendTotal;
extern char g_cmd;
extern int socket_cmd;
extern int socket_return;
extern BOOL bCameraStatus[];

extern char dataSendTotal;
extern unsigned int ipaddr_int[NUM_CAMARAS_MAX];
extern unsigned int num_of_persons[NUM_CAMARAS_MAX];

int capture_pic(long index);
int device_capture(LONG lUserID, const char *arg);
void * socket_thread(void *arg);

#endif
