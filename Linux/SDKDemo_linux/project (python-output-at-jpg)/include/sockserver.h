
#ifndef _SOCKSERVER_H_
#define _SOCKSERVER_H_

#define MAX_BYTES   2048
#define SOCKET_PORT 8888

extern const char *IP_FORMAT;

#define NUM_CAMARAS_MAX   120 //2020.04 for modbus TCP suport <= 123
#define NUM_CAMARAS   90 //2019.11.11

#define IP_VALID_START    ((unsigned short)2)
#define IP_VALID_END    ((unsigned short)NUM_CAMARAS+IP_VALID_START-1)


#include "Typedef.h"

extern int dataSendTotal;
extern char g_cmd;
extern int socket_cmd;
extern char socket_return;
extern BOOL bCameraStatus[];

extern int dataSendTotal;
extern unsigned int ipaddr_int[NUM_CAMARAS];
extern unsigned int num_of_persons[NUM_CAMARAS];

unsigned int IPtoInt(char *str_ip);
int capture_pic(long index);
int device_capture(LONG lUserID, const char *arg);
void * socket_thread(void *arg);

#endif
