/*
   Copyright (c) 2000-2001 BrainzSquare, Inc.
   mutext.h - mutual exclusion
*/

#ifndef _WIN32
#include <pthread.h>
#else
#include <windows.h>
#endif

#ifndef __MUTEXT_H__
#define __MUTEXT_H__

#include "ref.h"

//! 간단하게 구현된 Mutex 객체이다.
class CMutext : public CRef {
protected:
	int		m_bLocked;
#ifdef _WIN32
	CRITICAL_SECTION	m_cs;
#else
	pthread_mutex_t	m_mutex;	
#endif

public:
	CMutext();
	virtual ~CMutext();

	int  IsLocked();
	void Lock();
	void Unlock();
};

#endif
