/*
 * Copyright (c) 2003-2005 Brainzsquare, Inc.
 * debug.cpp - made by azrael
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "svcapp.h"
#include "debug.h"
#include "snprintf.h"

void _TRACE(const char* format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    (void) vfprintf(stderr, format, arglist);
    va_end(arglist);
    (void) abort;
}

char*   _TRACEINFO::m_pFileName;
int     _TRACEINFO::m_nLineNum;

void _TRACEINFO::_Trace(const char* format, ...)
{
	if(g_nSvcAppDebugLvl == 0) return;
#ifdef _WIN32
	va_list args;
	va_start(args, format);

	int nBuf;
	char szBuffer[512];
	
	mysnprintf(szBuffer, sizeof(szBuffer), "%s(%d): ", 
		_TRACEINFO::m_pFileName, _TRACEINFO::m_nLineNum);
	//OutputDebugString(szBuffer);
	fprintf(stderr,"%s",szBuffer);
	nBuf = myvsnprintf(szBuffer, sizeof(szBuffer), format, args);
	// was there an error? was the expanded string too long?
	assert(nBuf >= 0);
	
	//OutputDebugString(szBuffer);
	fprintf(stderr,"%s",szBuffer);
	va_end(args);
#else
    va_list arglist;
    va_start(arglist, format);
    fprintf(stderr,"%s(%d): ", _TRACEINFO::m_pFileName, _TRACEINFO::m_nLineNum);
    (void) vfprintf(stderr, format, arglist);
	va_end(arglist);
    (void) abort;
#endif // _WIN32
}
