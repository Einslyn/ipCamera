/***********************************************************************
** Copyright (C) Tongwei Video Technology Co.,Ltd. All rights reserved.
** Author       :	袁石维
** Date         : 2010-12-30
** Name         : DDPublic.cpp
** Version      : 1.0
** Description  :
** Modify Record:
***********************************************************************/
#include "DDPublic.h"
//#include "SWL_Public.h"

#ifdef __ENVIRONMENT_WIN32__
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

//+++++++++++++++++++++++++++++++++++++线程相关++++++++++++++++++++++++++++++++++
//输入参数：start_routine 线程的执行函数
//			pParam  线程执行时传入的参数
//			pRun    线程是否执行的bool量的指针，如果为NULL，则不理会此参数
//return value: 成功：返回线程ID    *pRun = true
//				失败：返回PUB_CREATE_THREAD_FAIL  *pRun的值不变
PUB_thread_t PUB_CreateThread(PUB_start_routine start_routine, void* pParam, bool *pRun)
{	
	PUB_thread_t threadID;
	if (NULL != pRun) 
	{
		*pRun = true;
	}
#ifdef __ENVIRONMENT_WIN32__
	threadID = CreateThread(0, 0, start_routine, (LPVOID)pParam, 0, NULL);
	if ( (PUB_CREATE_THREAD_FAIL == threadID) && (NULL != pRun) ) {
		*pRun = false;
	}
#elif defined __ENVIRONMENT_LINUX__
	int iRet = 0;
	if((iRet = pthread_create(&threadID, 0, start_routine, pParam)) != 0){
		threadID = PUB_CREATE_THREAD_FAIL;
		if (NULL != pRun)
		{
			*pRun = false;
		}		
	}
	errno = iRet;	//线程创建不会设置错误码，而是直接返回错误码
#endif
	return threadID;
}

//输入参数：start_routine 线程的执行函数
//			pParam  线程执行时传入的参数
//			pRun    线程是否执行的bool量的指针，如果为NULL，则不理会此参数
//			policy  线程调度优先级
//			return value: 成功：返回线程ID    *pRun = true
//				失败：返回PUB_CREATE_THREAD_FAIL  *pRun的值不变
PUB_thread_t PUB_CreateThreadEx(PUB_start_routine start_routine, void* pParam, bool *pRun, int policy)
{	
	PUB_thread_t threadID;
	if (NULL != pRun) 
	{
		*pRun = true;
	}
#ifdef __ENVIRONMENT_WIN32__
	threadID = CreateThread(0, 0, start_routine, (LPVOID)pParam, 0, NULL);
	if ( (PUB_CREATE_THREAD_FAIL == threadID) && (NULL != pRun) ) {
		*pRun = false;
	}
#elif defined __ENVIRONMENT_LINUX__
	int iRet = 0;
	pthread_attr_t thread_attr;
	struct sched_param thread_param;

	pthread_attr_init(&thread_attr);
	pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
	thread_param.sched_priority = policy;
	pthread_attr_setschedparam(&thread_attr, &thread_param);

	iRet = pthread_create(&threadID, &thread_attr, start_routine, pParam);
	if (0 != iRet)
	{
		threadID = PUB_CREATE_THREAD_FAIL;
		if (NULL != pRun)
		{
			*pRun = false;
		}		
	}

	errno = iRet;	//线程创建不会设置错误码，而是直接返回错误码
#endif
	return threadID;
}
//ThreadID 线程ID
//pRun     线程是否执行的bool量的指针
//return value: 退出线程后 *pThreadID = PUB_THREAD_ID_NOINIT，*pRun = false
void PUB_ExitThread(PUB_thread_t *pThreadID, bool *pRun)
{
	assert(NULL != pThreadID);
	if (PUB_THREAD_ID_NOINIT == *pThreadID)
	{
		return;
	}

	if (NULL != pRun) 
	{
		*pRun = false;
	}	
#ifdef    __ENVIRONMENT_WIN32__
	WaitForSingleObject(*pThreadID, INFINITE);
	CloseHandle(*pThreadID);
#elif defined __ENVIRONMENT_LINUX__
	if(pthread_self() == *pThreadID)
		CThreadManager::ExitThread(*pThreadID);
	else
		pthread_join(*pThreadID, NULL);
#endif
	*pThreadID = PUB_THREAD_ID_NOINIT;
}
//+++++++++++++++++++++++++++++++++++++线程相关++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++休眠和错误相关++++++++++++++++++++++++++++
//以为毫秒为单位的睡眠，
//BUGS：
//这里忽略了liunx下usleep是会返回非0的，并且没有了linux下不能睡眠微秒了
//linux下:SUSv2 version usleep 被信号中断，或者设置休息的时间大于一秒则会返回-1
//		  BSD version   usleep 无返回值
//		  dwMillisecond超过6分钟左右好像有问题 估计unsigned int不支持这么大的数字
void PUB_Sleep(unsigned int dwMillisecond)
{
#ifdef         __ENVIRONMENT_WIN32__
	if (0 == dwMillisecond) {
		Sleep(10);
	}
	else{
		Sleep(dwMillisecond);
	}

#elif defined  __ENVIRONMENT_LINUX__
	int iSec = dwMillisecond / 1000;
	int	iMicSec = (dwMillisecond % 1000) * 1000;

	//大于一秒的时间用这个睡眠
	if (iSec > 0) 
	{
		do 
		{
			iSec = sleep(iSec);
		} while(iSec > 0); 
	}

	if(0 != usleep(iMicSec))
	{
		if (EINTR == errno) 
		{
			printf("the usleep Interrupted by a signal. pid = %d\n", getpid());
		}
		else if (EINVAL == errno) 
		{
			assert(false);
			printf("the usleep param is not smaller than 1000000");
		}
	}
	//	usleep(dwMillisecond*1000);
#endif
}

//打印致命错误
void PUB_PrintError(const char* pFile, int iLine)
{
	char szErrorSource[200] = {0};
	sprintf(szErrorSource, "%s %d ", pFile, iLine);
	perror(szErrorSource);
}
//+++++++++++++++++++++++++++++++++++++休眠和错误相关++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++锁相关++++++++++++++++++++++++++++++++++++
//初始化锁，初始化后是可嵌套的锁
void PUB_InitLock(PUB_lock_t *pLock)
{
	assert(NULL != pLock);
#ifdef         __ENVIRONMENT_WIN32__
	InitializeCriticalSection(pLock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(pLock, &mutexAttr);
	pthread_mutexattr_destroy(&mutexAttr);
#endif
}

//销毁锁
void PUB_DestroyLock(PUB_lock_t *pLock)
{
	assert(NULL != pLock);
#ifdef         __ENVIRONMENT_WIN32__
	DeleteCriticalSection(pLock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_mutex_destroy(pLock);
#endif
}

CPUB_LockAction::CPUB_LockAction(PUB_lock_t* pLock)
{
	assert(NULL != pLock);
	m_pLock = pLock;
#ifdef         __ENVIRONMENT_WIN32__
	EnterCriticalSection(pLock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_mutex_lock(pLock);
#endif
}

CPUB_LockAction::~CPUB_LockAction()
{
#ifdef         __ENVIRONMENT_WIN32__
	LeaveCriticalSection(m_pLock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_mutex_unlock(m_pLock);
#endif
}

CPUB_Lock::CPUB_Lock()
{
#ifdef         __ENVIRONMENT_WIN32__
	InitializeCriticalSection(&m_Lock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_Lock, &mutexAttr);
	pthread_mutexattr_destroy(&mutexAttr);
	pthread_cond_init(&m_cond, NULL);
#endif
}

CPUB_Lock::~CPUB_Lock()
{
#ifdef         __ENVIRONMENT_WIN32__
	DeleteCriticalSection(&m_Lock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_cond_destroy(&m_cond);
	pthread_mutex_destroy(&m_Lock);
#endif
}

void CPUB_Lock::Lock()
{
#ifdef         __ENVIRONMENT_WIN32__
	EnterCriticalSection(&m_Lock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_mutex_lock(&m_Lock);
#endif
}

void CPUB_Lock::UnLock()
{
#ifdef         __ENVIRONMENT_WIN32__
	LeaveCriticalSection(&m_Lock);
#elif defined  __ENVIRONMENT_LINUX__
	pthread_mutex_unlock(&m_Lock);
#endif
}

#ifdef	__ENVIRONMENT_LINUX__
void CPUB_Lock::CondWait()
{
	pthread_cond_wait(&m_cond, &m_Lock);
}

void CPUB_Lock::CondSignal()
{
	pthread_cond_signal(&m_cond);
}

//return value: 0 成功 -1 加锁失败
int CPUB_Lock::TryLock()
{
	if(0 == pthread_mutex_trylock(&m_Lock))
	{
		return 0;
	}
	return -1;
}

//return value:	0 成功得到锁 1 超时 2 其他错误
//BUGS:这个在uclibc上好像没有用，或者是自己就写了一个bug
int CPUB_Lock::TimeLock(int iMicrosecond)
{
	struct timeval     timeNow;
	struct timespec    Timeout;
	gettimeofday(&timeNow, NULL);
	Timeout.tv_sec = timeNow.tv_sec;
	Timeout.tv_nsec = (timeNow.tv_usec + iMicrosecond) * 1000;              
	int iRet = pthread_mutex_timedlock(&m_Lock, &Timeout);
	if (0 == iRet) 
	{
		return 0;
	}
	else if (-1 == iRet) 
	{
		if (ETIMEDOUT == errno) 
		{
			return 1;
		}
		else
		{
			assert(false);
			return 2;
		}
	}
	return 0;
}
#endif

CWaitEvent::CWaitEvent()
{

}

CWaitEvent::~CWaitEvent()
{

}

void CWaitEvent::CreateWaitEvent()
{
#ifdef __ENVIRONMENT_WIN32__
	m_Event = CreateEvent(NULL, true, false, NULL);  
#else
	pthread_mutex_init(&m_lock, NULL);
	pthread_cond_init(&m_Event, NULL);
#endif
}

unsigned int CWaitEvent::WaitForCond(DWORD dwMilliseconds)
{
#ifdef __ENVIRONMENT_WIN32__
	unsigned int ret = WaitForSingleObject(m_Event, dwMilliseconds);
	ResetEvent(m_Event);

	return ret;
#else
	if(dwMilliseconds == INFINITE)
	{
		pthread_mutex_lock(&m_lock);
		pthread_cond_wait(&m_Event, &m_lock);
		pthread_mutex_unlock(&m_lock);
	}
	else
	{
		struct timespec ts;
		Gettimespec(&ts, dwMilliseconds);
		pthread_mutex_lock(&m_lock);
		pthread_cond_timedwait(&m_Event, &m_lock, &ts);
		pthread_mutex_unlock(&m_lock);
	}

	return 0;
#endif	
}

void CWaitEvent::SetEvent()
{
#ifdef	__ENVIRONMENT_WIN32__ 
	::SetEvent(m_Event);
#else
	pthread_cond_signal(&m_Event);
#endif
}

void CWaitEvent::PulseEvent()
{
#ifdef __ENVIRONMENT_WIN32__
	::PulseEvent(m_Event);  
#else
	pthread_cond_broadcast(&m_Event);
#endif
}

#ifndef __ENVIRONMENT_WIN32__
void CWaitEvent::Gettimespec(struct timespec* ptimespec, int offset) 
{ 
	struct timeval   now; 
	struct timespec  *ptime = (struct timespec *)ptimespec; 

	gettimeofday(&now, NULL); 
	ptime->tv_sec = now.tv_sec; 

	int tmp = now.tv_usec + offset * 1000; ////tmp是us级别的
	ptime->tv_sec = ptime->tv_sec + (tmp/(1000 * 1000));
	ptime->tv_nsec = (tmp % (1000 * 1000)) * 1000; 
} 
#endif

void CWaitEvent::Close()
{
#ifdef  __ENVIRONMENT_WIN32__
	CloseHandle(m_Event);
	m_Event = NULL;
#else
	pthread_cond_destroy(&m_Event);
	pthread_mutex_destroy(&m_lock);
#endif
}

//+++++++++++++++++++++++++++++++++++++锁相关++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++时间相关++++++++++++++++++++++++++++++++++
/*
*
* 输入参数说明：在Window下传入的是FILETIME结构体，而且精确到us。在Linux下传入的是timeval，也是精确到us。
* 返回值说明：在Window下和Linux下返回的值都是自1970年0:0:0开始的绝对时间,数量级是us。
*
*/
LONGLONG TimeToDVRTime(DVRDATETIME  DVRtm)   
{
#ifdef __ENVIRONMENT_WIN32__
	LONGLONG  AdjustTime = 116444736000000000;
	LONGLONG  Temptm = *((LONGLONG *)&DVRtm) ;
	Temptm -= AdjustTime;
	Temptm /= 10;
#else
	LONGLONG  Temptm = (LONGLONG)DVRtm.tv_sec*1000000 + (LONGLONG)DVRtm.tv_usec;
#endif
	return Temptm;
}


unsigned int TimeToDVRTime32(DVRDATETIME  DVRtm)   
{
#ifdef __ENVIRONMENT_WIN32__
	LONGLONG  AdjustTime = 116444736000000000;
	LONGLONG  Temptm = *((LONGLONG *)&DVRtm) ;
	Temptm -= AdjustTime;
	Temptm /= 10;
#else
	LONGLONG  Temptm = (LONGLONG)DVRtm.tv_sec*1000000 + (LONGLONG)DVRtm.tv_usec;
#endif

	return (unsigned int)(Temptm / 1000000);
}

//从标准tm到LONGLONG的转换
LONGLONG TmToDVRTime(tm time)  
{
#ifdef __ENVIRONMENT_WIN32__
	LONGLONG temp;
	if(time.tm_isdst == 0)
	{
		CTime ctime(time.tm_year+1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
		temp = CTmTo64Bit(ctime);
	}
	else
	{
		CTime ctime(time.tm_year+1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec,time.tm_isdst);
		temp = CTmTo64Bit(ctime);
	}
	return temp;
#else

	time_t tt=mktime(&time);
	LONGLONG temp=(LONGLONG)tt*1000000;
	return temp;
#endif
}

unsigned int TmToDVRTime32(tm time)  
{
#ifdef __ENVIRONMENT_WIN32__
	unsigned int temp;

	if(time.tm_isdst == 0)
	{
		CTime ctime(time.tm_year+1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
		temp = CTmTo32Bit(ctime);
	}
	else
	{
		CTime ctime(time.tm_year+1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec,time.tm_isdst);
		temp = CTmTo32Bit(ctime);
	}

	return temp;
#else
	time.tm_isdst = -1;						//tm_isdat = -1时mktime()转换当前时间到GMT时间的方式为:
	//1: time 在DST时间内时localtime - timezone - dstoffset 2: time不在DST时间内时localtime - timezone

	//tm_isdst = 0 时mktime()转换当前时间到GMT时间的方式为:localtime - timezone 

	//tm_isdst = 1 时mktime()转换当前时间到GMT时间的方式为:localtime - timezone - dstoffset
	return (unsigned int)(mktime(&time));
#endif
}

#ifdef __ENVIRONMENT_WIN32__
LONGLONG CTmTo64Bit(CTime time)
{
#ifdef HAS_TIME_ZONE
	return ((LONGLONG)time.GetTime())*1000000;
#else
	LONGLONG   RetTime;
	SYSTEMTIME sysTime;
	FILETIME fileTime;

	time.GetAsSystemTime(sysTime);
	SystemTimeToFileTime(&sysTime, &fileTime);

	RetTime = TimeToDVRTime(fileTime);

	return RetTime; 
#endif
}

unsigned int CTmTo32Bit(CTime time)
{
#ifdef HAS_TIME_ZONE
	return time.GetTime();
#else
	SYSTEMTIME sysTime;
	FILETIME fileTime;

	time.GetAsSystemTime(sysTime);
	SystemTimeToFileTime(&sysTime, &fileTime);

	return TimeToDVRTime32(fileTime);
#endif
}

CTime _64BitToCTm(LONGLONG time)
{
#ifdef HAS_TIME_ZONE
	return CTime(time/1000000);
#else
	LONGLONG  AdjustTime = 116444736000000000;
	time *= 10;
	time += AdjustTime;
	FILETIME TempTime = *((FILETIME *)&time);
	SYSTEMTIME sysTime;
	FileTimeToSystemTime(&TempTime, &sysTime);
	return CTime(sysTime);
#endif
}

CTime _32BitToCTm(unsigned int time)
{
#ifdef HAS_TIME_ZONE
	return CTime(time);
#else
	LONGLONG  AdjustTime = 116444736000000000;
	LONGLONG tTime = (LONGLONG)(time) * 1000000;
	tTime *= 10;
	tTime += AdjustTime;

	FILETIME TempTime = *((FILETIME *)&tTime);
	SYSTEMTIME sysTime;
	FileTimeToSystemTime(&TempTime, &sysTime);

	return CTime(sysTime);
#endif
}
#endif
/*
*
* 输入参数说明：在Window下和Linux下值都是自1970年0:0:0开始的绝对时间，精确到us的
* 返回值说明：在Window和Liunx下返回的都是tm类型的结构体，而且只精确到s。
*
*/


tm DVRTimeToTm(LONGLONG DVRtm)
{
	//static CPUB_Lock  s_TimeLock;

#ifdef __ENVIRONMENT_WIN32__
	//s_TimeLock.Lock();
	CTime ctime = _64BitToCTm(DVRtm);
	struct tm DisplayTime;
	ctime.GetLocalTm(&DisplayTime);
	//s_TimeLock.UnLock();
	return DisplayTime;
#else

	struct tm   DisplayTime, *ptime; 
	time_t      SecNum;
	SecNum = (int)(DVRtm / 1000000);
	//s_TimeLock.Lock();
	ptime = localtime(&SecNum);
	DisplayTime = *ptime;
	//s_TimeLock.UnLock();
	DisplayTime.tm_year = DisplayTime.tm_year;
	return DisplayTime;
#endif
}

tm DVRTime32ToTm(unsigned int DVRtm)
{
	static CPUB_Lock  s_TimeLock;

#ifdef __ENVIRONMENT_WIN32__
	s_TimeLock.Lock();
	CTime ctime = _32BitToCTm(DVRtm);
	struct tm DisplayTime;
	ctime.GetLocalTm(&DisplayTime);
	s_TimeLock.UnLock();
	return DisplayTime;
#else

	struct tm   DisplayTime, *ptime; 
	time_t      SecNum = DVRtm;

	s_TimeLock.Lock();
	ptime = localtime(&SecNum);
	DisplayTime = *ptime;
	s_TimeLock.UnLock();

	DisplayTime.tm_year = DisplayTime.tm_year;
	return DisplayTime;
#endif
}

#ifndef __ENVIRONMENT_WIN32__
CThreadManager g_ThreadManager;
CThreadManager::CThreadManager()
{
	m_ThreadManagerExit=false;
	m_ThreadManagerThread= PUB_CreateThread(ThreadManagerProc, this, NULL);
}
CThreadManager::~CThreadManager()
{
	m_ThreadManagerExit=true;
	//	PUB_ExitThread(&m_ThreadManagerThread, NULL);
}
void CThreadManager::ExitThread(PUB_thread_t handle)
{
	g_ThreadManager.m_ThreadListLock.Lock();
	g_ThreadManager.m_ThreadList.push_back(handle);
	g_ThreadManager.m_ThreadListLock.UnLock();
}
RESULT WINAPI CThreadManager::ThreadManagerProc(LPVOID lpParameter)
{
	CThreadManager *pManager=(CThreadManager*)lpParameter;
	pManager->ThreadManagerFunc();
	return 0;
}
void CThreadManager::ThreadManagerFunc()
{
	PUB_thread_t handle=0;

	while(!m_ThreadManagerExit)
	{
		handle=0;
		m_ThreadListLock.Lock();
		if(m_ThreadList.size() > 0)
		{
			handle = m_ThreadList.front();
			m_ThreadList.pop_front();
		}
		m_ThreadListLock.UnLock();
		if(handle)
		{
			PUB_ExitThread(&handle, NULL);
		}

		PUB_Sleep(100);
	}
}
#endif

LONGLONG GetCurrTime()
{
	LONGLONG time;
#ifdef __ENVIRONMENT_WIN32__
	SYSTEMTIME systemtime;
	FILETIME  filetime;
	GetSystemTime(&systemtime);
	if (SystemTimeToFileTime(&systemtime, &filetime))
	{
		time = TimeToDVRTime(filetime);
	}
#else
	timeval tp;
	if (0 == gettimeofday(&tp, NULL))
		time = TimeToDVRTime(tp);
#endif
	return time;
}//GetCurrTime end.

unsigned int GetCurrTime32()
{
	unsigned int time;

#ifdef __ENVIRONMENT_WIN32__
	SYSTEMTIME systemtime;
	FILETIME  filetime;
	GetSystemTime(&systemtime);
	if (SystemTimeToFileTime(&systemtime, &filetime))
	{
		time = TimeToDVRTime32(filetime);
	}
#else
	timeval tp;
	if (0 == gettimeofday(&tp, NULL))
		time = TimeToDVRTime32(tp);
#endif

	return time;
}

LONGLONG GetZeroTimeOfDate(LONGLONG time)
{
	tm display = DVRTimeToTm(time);

	display.tm_hour = 0;
	display.tm_sec = 0;
	display.tm_min = 0;

	return TmToDVRTime(display);
}

unsigned int GetZeroTime32OfDate(unsigned int time)
{
	tm display = DVRTime32ToTm(time);

	display.tm_hour = 0;
	display.tm_sec = 0;
	display.tm_min = 0;

	return TmToDVRTime32(display);
}

LONGLONG GetEndTimeOfDate(LONGLONG time)
{
	tm display = DVRTimeToTm(time);

	display.tm_hour = 23;
	display.tm_sec = 59;
	display.tm_min = 59;

	return TmToDVRTime(display) + 999999;
}

unsigned int GetEndTime32OfDate(unsigned int time)
{
	tm display = DVRTime32ToTm(time);

	display.tm_hour = 23;
	display.tm_sec = 59;
	display.tm_min = 59;

	return TmToDVRTime32(display);
}

//noTime false 时间 true 数字
void PrintfBit64(ULONGLONG n_num, bool noTime)
{
	if (noTime)
	{
		unsigned int h = (ULONG)(0x00000000ffffffff&(n_num>>32));
		unsigned int l = (ULONG)(0x00000000ffffffff&n_num);
		printf("%8x %8x\n", h, l);
	}
	else
	{
		tm time = DVRTimeToTm(n_num);
		int ms = static_cast<int>(n_num%1000000);

		printf("%d-%d-%d %d:%d:%d ms:%d\n", time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, ms);
	}
}

void PrintfBit32Time(unsigned int timeSec)
{
	tm time = DVRTime32ToTm(timeSec);
	printf("%d-%d-%d %d:%d:%d\n", time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
}
//+++++++++++++++++++++++++++++++++++++时间相关++++++++++++++++++++++++++++++++++

//end


//+++++++++++++++++++++++++++++++++++++视频相关++++++++++++++++++++++++++++++++++
//根据关键帧，判断出该帧的宽高
//只支持16像素整数倍图像大小，非high profile的视频流图像大小检测。
typedef signed char  int8_ts;
typedef unsigned char  uint8_tu;
const uint8_tu ff_ue_golomb_vlc_codet[512] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
	7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
const uint8_tu ff_golomb_vlc_lent[512] = 
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};
static const int8_ts ff_se_golomb_vlc_code[512] =
{
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, -8,  9, -9, 10,-10, 11,-11, 12,-12, 13,-13, 14,-14, 15,-15,
	4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,  6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

int av_log22(unsigned int v)
{
	int n;
	static const uint8_tu ff_log2_tab[256]={
		0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
	};

	n = 0;
	if (v & 0xffff0000) {
		v >>= 16;
		n += 16;
	}
	if (v & 0xff00) {
		v >>= 8;
		n += 8;
	}
	n += ff_log2_tab[v];

	return n;
}
int get_ue_golomb1(unsigned char* buff,int &index_r)
{
	unsigned int buf;
	int log;
	int index = index_r;
	buff = buff+(index >>3);
	index = index & 7;
	buf = ((buff[0]<<index)&0xff)+(buff[1]>>(8-index));
	buf <<=8;
	buf += ((buff[1]<<index)&0xff)+(buff[2]>>(8-index));
	buf <<=8;
	buf += ((buff[2]<<index)&0xff)+(buff[3]>>(8-index));
	buf <<=8;
	buf += ((buff[3]<<index)&0xff)+(buff[4]>>(8-index));

	if(buf >= (1<<27))
	{
		buf >>= 32 - 9;
		index_r += ff_golomb_vlc_lent[buf];
		return ff_ue_golomb_vlc_codet[buf];
	}
	else
	{
		log= 2*av_log22(buf) - 31;
		buf>>= log;
		buf--;
		//  LAST_SKIP_BITS(re, gb, 32 - log);
		index_r +=32 - log;
		//      CLOSE_READER(re, gb);
		/**/
		return buf;
	}
}

int get_se_golomb1(unsigned char* buff,int &index_r)
{
	unsigned int buf;
	int log;
	int index = index_r;
	buff = buff+(index >>3);
	index = index & 7;
	buf = ((buff[0]<<index)&0xff)+(buff[1]>>(8-index));
	buf <<=8;
	buf += ((buff[1]<<index)&0xff)+(buff[2]>>(8-index));
	buf <<=8;
	buf += ((buff[2]<<index)&0xff)+(buff[3]>>(8-index));
	buf <<=8;
	buf += ((buff[3]<<index)&0xff)+(buff[4]>>(8-index));

	if(buf >= (1<<27)){
		buf >>= 32 - 9;
		index_r += ff_golomb_vlc_lent[buf];
		return ff_se_golomb_vlc_code[buf];
	}else{
		log= 2*av_log22(buf) - 31;
		buf>>= log;
		index_r +=  32 - log;
		if(buf&1) buf= -(buf>>1);
		else      buf=  (buf>>1);
		return buf;
	}
}

int getvideosize(unsigned char* buf,int buf_size,int& width,int & height)
{
	int buf_index = 0;

	for(; buf_index + 3 < buf_size; buf_index++)
	{
		if(buf[buf_index] == 0 && buf[buf_index+1] == 0 && buf[buf_index+2] == 1&& buf[buf_index+3] == 0x67)
		{
			break;
		}
	}
	if(buf_index>=buf_size-3)
	{
		return 0;
	}
	unsigned char * sps = &buf[buf_index+4];
	if(sps[0]>100)
	{
		return 0;//high profile 
	}
	int index=24;
	char sps_id = get_ue_golomb1(sps,index);
	index = 26;
	char poc_type = get_ue_golomb1(sps,index);
	if(poc_type == 0)
	{ //FIXME #define   
		//  sps->log2_max_poc_lsb= get_ue_golomb(&s->gb,hDec) + 4;//27
		index = 30;
	} else if(poc_type == 1)
	{//FIXME #define         //30
		//   sps->delta_pic_order_always_zero_flag= get_bits1(&s->gb);
		index ++;
		get_se_golomb1(sps,index);
		get_se_golomb1(sps,index);
		int poc_cycle_length= get_ue_golomb1(sps,index);

		if(poc_cycle_length >= 255)
		{
			//   av_log(h->s.avctx, AV_LOG_ERROR, "poc_cycle_length overflow %u\n", tmp);
			return -1;
		}

		for(int i=0; i<poc_cycle_length; i++)
			get_se_golomb1(sps,index);/**/
	}
	else if(poc_type != 2)
	{

		return -1;
	}
	int ref= get_ue_golomb1(sps,index);//29 
	index ++;
	width= get_ue_golomb1(sps,index) + 1;//33
	width*=16;
	height= get_ue_golomb1(sps,index) + 1;
	height*=16;
	return 1;
}