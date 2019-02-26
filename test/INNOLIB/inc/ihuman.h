/*
   Copyright (c) 2002-2005 BrainzSquare, Inc.
   ihuman.h - human readable
   2005.05.24. options : comman, 1000, 1024
   2005.05.23. divided by 1000 -> divided by 1024
 */

#include <time.h>

#ifndef __IHUMAN_H__
#define __IHUMAN_H__

#define IHUMAN_COMMA	0		// 1,234,456
#define IHUMAN_1000		1000	// 1.23K, ... (divided by 1000) -- default
#define IHUMAN_1024		1024	// 999M, ... (divided by 1024)

/////////////////////////////////////////////
// integer 64 bits
// format; Windows %I64d, Linux/Unix %lld
//
#ifdef _WIN32
typedef LONGLONG	myint64;
#else
typedef long long	myint64;
#endif

//! 입력한 64bit integer에 대해서 단위를 가지는 문자열 값으로 변환한다.
char* IHumanReadable(myint64 val,char *buf,int len,int nHuman=IHUMAN_1000, int cipher = 2);
//! 입력한 32bit integer에 대해서 단위를 가지는 문자열 값으로 변환한다.
char* IHumanReadable(int val,char *buf,int len,int nHuman=IHUMAN_1000, int cipher = 2);
//! 입력한 unsigned integer에 대해서 단위를 가지는 문자열 값으로 변환한다.
char* IHumanReadable(unsigned int val,char *buf,int len,int nHuman=IHUMAN_1000, int cipher = 2);
//! 입력한 long에 대해서 단위를 가지는 문자열 값으로 변환한다.
char* IHumanReadable(long val,char *buf,int len,int nHuman=IHUMAN_1000, int cipher = 2);
//! 입력한 unsigned long에 대해서 단위를 가지는 문자열 값으로 변환한다.
char* IHumanReadable(unsigned long val,char *buf,int len,int nHuman=IHUMAN_1000);
//! 입력한 double에 대해서 단위를 가지는 문자열 값으로 변환한다.
char* IHumanReadable(double val,char *buf,int len,int nHuman=IHUMAN_1000, int cipher = 2);
//! 입력한 float에 대해서 단위를 가지는 문자열 값으로 변환한다.
char* IHumanReadable(float val,char *buf,int len,int nHuman=IHUMAN_1000, int cipher = 2);

//! 입력한 단위를 가지는 문자열값을 숫자값으로 변환한다.
void  IHumanToDigit(char* buf,myint64& val,int nHuman=IHUMAN_1000);
//! 입력한 단위를 가지는 문자열값을 숫자값으로 변환한다.
void  IHumanToDigit(char* buf,unsigned int& val,int nHuman=IHUMAN_1000);

//! 입력한 time_t 타입의 데이터를 문자열 형식으로 변환한다. (ex) 2004/02/04 12:00:24
char* IHumanSecToDate(time_t ttime,char* buf,int len=32);
//! 입력한 time_t 타입의 데이터를 문자열 형식으로 변환한다. (ex) 7 day(s), 12h 0m 24s
char* IHumanSecToTime(time_t ttime,char* buf,int len=32);

#if		(_HPUX || _OSF1)
myint64 strtoll(const char* str,char** ptr,int base);
#endif

#endif /* __IHUMAN_H__ */
