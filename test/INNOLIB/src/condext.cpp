/*
   Copyright (c) 2002 Brainzsquare, Inc.
   condext.cpp - conditional wait, signal
 */

#include "stdafx.h"

#ifndef _WIN32
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#endif

#include "condext.h"

static unsigned int _nCondextNum = 0;

CCondext::CCondext(int bManual)
{
    _nCondextNum++;
#ifdef _WIN32
    //char name[100];
    //mysnprintf(name,100,"Clique_Thread_Condext_%03d",_nCondextNum);
    m_cond = CreateEvent(NULL,bManual,FALSE,NULL);
#else
    pthread_cond_init(&m_cond,NULL);
#endif
}

CCondext::~CCondext()
{
#ifdef _WIN32
    if(m_cond) CloseHandle(m_cond);
#else
    pthread_cond_destroy(&m_cond);
#endif
}

void CCondext::Wait()
{
    // wait for signal
#ifdef _WIN32
    Unlock();
    if(m_cond) WaitForSingleObject(m_cond,INFINITE);
    Lock();
#else
    pthread_cond_wait(&m_cond,&m_mutex);	
#endif
}

int CCondext::TimedWait(unsigned int uTimeout)
{
    int status = CONDEXT_STATUS_OK;
    // timed wait for signal
#ifdef _WIN32
    Unlock();
    if(m_cond) {
		DWORD dwRet = WaitForSingleObject(m_cond,uTimeout);
		if(dwRet == WAIT_TIMEOUT) status = CONDEXT_STATUS_TIMEO;
    }
    Lock();    
#else
    struct timeval tv;
    struct timezone tz;
    struct timespec ts;
    gettimeofday(&tv,&tz);
    ts.tv_sec = tv.tv_sec + (long)(uTimeout/1000.0);
    ts.tv_nsec = tv.tv_usec*1000 + (long)((uTimeout%1000)*1000000);
	// begin: requested by cmmoon (2005.03.21.)
	ts.tv_sec  += ts.tv_nsec / 1000000000;
	ts.tv_nsec %= 1000000000;
	// end: requested by cmmoon (2005.03.21.)
    int nRet = pthread_cond_timedwait(&m_cond,&m_mutex,&ts);
    if(nRet != 0) status = CONDEXT_STATUS_TIMEO;
#endif
    return status;
}

void CCondext::Signal()
{
#ifdef _WIN32
    Unlock();
    if(m_cond) SetEvent(m_cond);
    Lock();
#else
    pthread_cond_signal(&m_cond);	
#endif
}

void CCondext::Broadcast()
{
#ifdef _WIN32
    Unlock();
    if(m_cond) PulseEvent(m_cond);
    Lock();
#else
    pthread_cond_broadcast(&m_cond);
#endif
}

void CCondext::Reset()
{
#ifdef _WIN32
	if(m_cond) ResetEvent(m_cond);
#endif
}
