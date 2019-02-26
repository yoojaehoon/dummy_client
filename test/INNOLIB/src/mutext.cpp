/*
   Copyright (c) 2000-2002 Brainzsquare, Inc.
   mutext.cc - mutual exclusion
 */

#include "stdafx.h"

#include <stdio.h>
#include "mutext.h"

static unsigned int _nMutextNum = 0;

CMutext::CMutext()
{
    _nMutextNum++;
    m_bLocked = 0;
#ifdef _WIN32
	InitializeCriticalSection(&m_cs);
#elif _NCR
    pthread_mutex_init(&m_mutex,pthread_mutexattr_default);
#else
    pthread_mutex_init(&m_mutex,NULL);
#endif
}

CMutext::~CMutext()
{
#ifdef _WIN32
	DeleteCriticalSection(&m_cs);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

int CMutext::IsLocked()
{
    return m_bLocked;
}

void CMutext::Lock()
{
    m_bLocked = 1;
#ifdef _WIN32
	EnterCriticalSection(&m_cs);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void CMutext::Unlock()
{
#ifdef _WIN32
	LeaveCriticalSection(&m_cs);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
    m_bLocked = 0;
}
