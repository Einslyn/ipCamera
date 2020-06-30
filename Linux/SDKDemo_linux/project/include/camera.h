<<<<<<< HEAD
#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Typedef.h"

#define MAX_BYTES   2048
#define SOCKET_PORT 8888

extern char NUM_CAMARAS_MAX; //2020.04 for modbus TCP suport <= 123
extern char NUM_CAMARAS;

extern unsigned int IP_VALID_START;
extern unsigned int IP_VALID_END;

extern BOOL bNeedConnect[];
extern unsigned int arrayLoginID[];
extern char cameraIP[255][16];

int camera_online(int start, int end);
int capture_pic(long index);
int device_capture(LONG lUserID, const char *arg);
void *camera_thread(void *arg);

#endif
=======
#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Typedef.h"

#define MAX_BYTES   2048
#define SOCKET_PORT 8888

extern char NUM_CAMARAS_MAX; //2020.04 for modbus TCP suport <= 123
extern char NUM_CAMARAS;

extern unsigned int IP_VALID_START;
extern unsigned int IP_VALID_END;

extern BOOL bNeedConnect[];
extern unsigned int arrayLoginID[];
extern char cameraIP[255][16];

int camera_online(int start, int end);
int capture_pic(long index);
int device_capture(LONG lUserID, const char *arg);
void *camera_thread(void *arg);

#endif
>>>>>>> a6d2214bda1c223f2b5a1caf2eccb7f94d6903ae
