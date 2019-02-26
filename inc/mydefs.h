/*
   Copyright (c) 2002-2005 BrainzSquare, Inc.
   mydefs.h - definition
 */

#ifndef __MYDEFS_H__
#define __MYDEFS_H__

#ifndef BYTE
typedef unsigned char	BYTE;
#endif

#ifndef UINT
typedef unsigned int	UINT;
#endif

/* // commented by kskim 2009-6-1. conflicted by another library
#ifndef ULONG
typedef unsigned long	ULONG;
#endif
*/

#ifndef myint64
#ifdef _WIN32
typedef __int64				myint64;
typedef unsigned __int64	umyint64;
#else
typedef long long           myint64;
typedef unsigned long long  umyint64;
#endif
#endif

#ifndef mysleep
#ifdef _WIN32
#define mysleep(n)	Sleep(1000*(n))
#define mymsleep(n)	Sleep(n)
#else
#define mysleep(n)	sleep(n)
#define mymsleep(n)	usleep(1000*(n))
#endif
#endif

#ifndef myclose
#ifdef _WIN32
#define myclose(n)	closesocket(n)
#else
#define myclose(n)	close(n)
#endif
#endif

#ifdef _WIN32
#define inet_network(a)	ntohl(inet_addr(a))
#endif

// added by kskim 2006.4.10
#ifndef INFINITE
#define INFINITE 	(0xffffffff)
#endif

// modified by kskim 2006.4.10
// like system function
// nTimeout : milliseconds (default : INFINITE,not use timeout. it's mean block)
// if windows 
// return : -1 = cannot excute, 0 : timeout, 1 = success
// else
// return : !system(path)
int mysystem(char* path, unsigned int nTimeout = INFINITE);
int SetLimit(int nType,int nLimit);

#endif
