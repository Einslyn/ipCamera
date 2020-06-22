
#ifndef _SOCKSERVER_H_
#define _SOCKSERVER_H_

#define MAX_BYTES   2048
#define SOCKET_PORT 8888


extern char NUM_CAMARAS_MAX ; //2020.04 for modbus TCP suport <= 123
extern char NUM_CAMARAS ;

extern char IP_VALID_START ;
extern char IP_VALID_END ;

#define FILE_PATH "~/wuyi/tvtCamera_save/Linux/bin/"


#include "Typedef.h"

extern int dataSendTotal;
extern char g_cmd;
extern int socket_cmd;
extern char socket_return;
extern BOOL bCameraStatus[];

extern int dataSendTotal;
extern unsigned int ipaddr_int[];
extern unsigned int num_of_persons[];

int capture_pic(long index);
int device_capture(LONG lUserID, const char *arg);
void * socket_thread(void *arg);

#endif
