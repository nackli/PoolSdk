/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
* @LastEditTime: 2025-08-29
*/
/***************************************************************************************************************************************************/
#pragma once
#ifndef __EVENT_LINUX_WIN__H__
#define __EVENT_LINUX_WIN__H__
#ifdef _MSC_VER
#include <Windows.h>
#define EVENT_HANDLE HANDLE
#else
#include <pthread.h>
#include <sys/time.h>
#include <new>
#include <cerrno>
typedef struct  
{
    bool bState;
    bool bManualReset;
    pthread_mutex_t mtxThread;
    pthread_cond_t condThread;
}EVENT_T;
#define EVENT_HANDLE EVENT_T*
#endif

inline  EVENT_HANDLE eventCreate(bool bManualReset, bool bInitState , const char *szEventName = nullptr)
{
#ifdef _MSC_VER
    HANDLE hEvent = CreateEventA(NULL, bManualReset, bInitState, szEventName);
   	if (hEvent == nullptr && szEventName)
	{
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			hEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, szEventName);
	}
#else
    EVENT_HANDLE hEvent = new(std::nothrow) EVENT_T;
    if (hEvent == NULL)
        return NULL;
	
    hEvent->bState = bInitState;
    hEvent->bManualReset = bManualReset;
    if (pthread_mutex_init(&hEvent->mtxThread, NULL))
    {
        delete hEvent;
        return NULL;
    }
    if (pthread_cond_init(&hEvent->condThread, NULL))
    {
        pthread_mutex_destroy(&hEvent->mtxThread);
        delete hEvent;
        return NULL;
    }
#endif
    return hEvent;	
}

inline int eventWait(EVENT_HANDLE hEvent)
{
#ifdef _MSC_VER
    DWORD ret = WaitForSingleObject(hEvent, INFINITE);
    if (ret == WAIT_OBJECT_0)
        return 0;
    return -1;
#else
    if (pthread_mutex_lock(&hEvent->mtxThread))        
        return -1;   
	
    while (!hEvent->bState)    
    {      
        if (pthread_cond_wait(&hEvent->condThread, &hEvent->mtxThread))   
        {   
            pthread_mutex_unlock(&hEvent->mtxThread); 
            return -1;   
        }   
    } 
	
    if (!hEvent->bManualReset) 
        hEvent->bState = false;
	
    if (pthread_mutex_unlock(&hEvent->mtxThread))        
        return -1;   
	
    return 0;
#endif	
}

inline  int eventWaitTimeOut(EVENT_HANDLE hEvent, long milliseconds)
{
#ifdef _MSC_VER
    DWORD ret = WaitForSingleObject(hEvent, milliseconds);
    if (ret == WAIT_OBJECT_0)
        return 0;
	
    if (ret == WAIT_TIMEOUT)
        return 1;
	
    return -1;
#else
    int rc = 0;   
    struct timespec abstime;   
    struct timeval tv;   
    gettimeofday(&tv, NULL);   
    abstime.tv_sec  = tv.tv_sec + milliseconds / 1000;   
    abstime.tv_nsec = tv.tv_usec*1000 + (milliseconds % 1000)*1000000;   
    if (abstime.tv_nsec >= 1000000000)   
    {   
        abstime.tv_nsec -= 1000000000;   
        abstime.tv_sec++;   
    }   


    if (pthread_mutex_lock(&hEvent->mtxThread) != 0)       
        return -1;   
	
    while (!hEvent->bState)    
    {      
        rc = pthread_cond_timedwait(&hEvent->condThread, &hEvent->mtxThread, &abstime);
        if (rc == ETIMEDOUT) 
            break;   
        pthread_mutex_unlock(&hEvent->mtxThread);    
        return -1;   
    }  
	
    if (rc == 0 && !hEvent->bManualReset)   
        hEvent->bState = false;
	
    if (pthread_mutex_unlock(&hEvent->mtxThread) != 0)   		
        return -1;   


    if (rc == ETIMEDOUT)
        //timeout return 1
        return 1;
		
    //wait event success return 0
    return 0;
#endif	
}

inline  int eventSet(EVENT_HANDLE hEvent)
{
#ifdef _MSC_VER
    return !SetEvent(hEvent);
#else
    if (pthread_mutex_lock(&hEvent->mtxThread) != 0)
    {
        return -1;
    }


    hEvent->bState = true;


    if (hEvent->bManualReset)
    {
        if(pthread_cond_broadcast(&hEvent->condThread))
            return -1;
    }
    else
    {
        if(pthread_cond_signal(&hEvent->condThread))
            return -1;
    }


    if (pthread_mutex_unlock(&hEvent->mtxThread) != 0)
       return -1;


    return 0;
#endif
}

inline int eventReset(EVENT_HANDLE hEvent)
{
#ifdef _MSC_VER
    if (ResetEvent(hEvent))
        return 0;
    return -1;
#else
    if (pthread_mutex_lock(&hEvent->mtxThread) != 0)
        return -1;

    hEvent->bState = false;


    if (pthread_mutex_unlock(&hEvent->mtxThread) != 0)     
        return -1;
    return 0;
#endif
}

inline void eventDestroy(EVENT_HANDLE hEvent)
{
#ifdef _MSC_VER
    CloseHandle(hEvent);
#else
    pthread_cond_destroy(&hEvent->condThread);
    pthread_mutex_destroy(&hEvent->mtxThread);
    delete hEvent;
#endif
}
#endif