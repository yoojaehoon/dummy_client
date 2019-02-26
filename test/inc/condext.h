/*
  Copyright (c) 2002 Brainzsquare, Inc.
  condext.h - condition wait, signal
 */

#include "mutext.h"

#ifndef __CONDEXT_H__
#define __CONDEXT_H__

#define CONDEXT_STATUS_OK			0	// ok
#define CONDEXT_STATUS_TIMEO		1	// timed out

class CCondext : public CMutext {
public:
#ifdef _WIN32
	HANDLE		m_cond;
#else
	pthread_cond_t	m_cond;
#endif
    
public:
    CCondext(int bManual=0);
    virtual ~CCondext();

    void Wait();							// wait signal
	// TimedWait: return CONDEXT_STATUS_OK or CONDEXT_STATUS_TIMEO
    int  TimedWait(unsigned int uTimeout);	// milliseconds
    void Signal();							// single signal
    void Broadcast();						// broadcast signal
	void Reset();							// reset event
};

#endif
