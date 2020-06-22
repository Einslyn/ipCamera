
#ifndef _DISPLAY_DEFINE_H_
#define _DISPLAY_DEFINE_H_
#include "dvrdvstypedef.h"

//设计固定层数：标准分割层、画中画层
enum LAYOUT_LAYER
{
	LAYOUT_LAYER_0,
	LAYOUT_LAYER_1,
};

enum STREAM_TYPE
{
	STREAM_TYPE_NONE,
	STREAM_TYPE_LIVE,
	STREAM_TYPE_PLAYBACK = 100,
};

enum LAYOUT_PIP_POSITION
{
	LAYOUT_PIP_LEFT_TOP,
	LAYOUT_PIP_RIGHT_TOP,
	LAYOUT_PIP_LEFT_BOTTOM,
	LAYOUT_PIP_RIGHT_BOTTOM,
	LAYOUT_PIP_NUM,
};

enum CH_STATUS
{
	CH_STATUS_FREE		= 0,
	CH_STATUS_VLOSS		= 1,
	CH_STATUS_DISCONNECT= 2, 
	CH_STATUS_NO_AUTH	= 3,
	CH_STATUS_RECORD	= 4
};

const unsigned int MAX_NORMAL_PORT_NUM = 128;
const unsigned int	MAX_DISPLAY_PORT_NUM = MAX_NORMAL_PORT_NUM + LAYOUT_PIP_NUM;
const unsigned int PIP_SIZE_PERCENT = 20;

const unsigned int PORT_STATUS_BAR_HEIGHT = 16;

typedef void (CALLBACK *DRAW_FUN_CALLBACK)(POINTERHANDLE lLiveHandle, HDC hDC, void *pUser);

typedef struct _display_port
{
	unsigned short index;	//区域编号
	unsigned short zOrder;	//Z轴显示顺序LAYOUT_LAYER

	unsigned short xPos;	//左上角横坐标
	unsigned short yPos;	//左上角纵坐标
	unsigned short cx;		//显示区域宽度（像素）
	unsigned short cy;		//显示区域高度（像素）

	unsigned short channel;
	unsigned short streamType;//STREAM_TYPE
	unsigned int deviceID;
	unsigned int liveHandle;
	DRAW_FUN_CALLBACK	drawCallBackFunc;
	void *				cbParam;
}DISPLAY_PORT;

#endif
