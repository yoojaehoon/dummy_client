/*
   Copyright (c) 2003 BrainzSquare, Inc.
   stat64.h - stat 64bit operation
 */

#ifndef __STAT64_H__
#define __STAT64_H__

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#define S_ISLNK(st_mode)    (0)
#define S_ISREG(st_mode)    ((st_mode) & _S_IFREG)
#define S_ISDIR(st_mode)    ((st_mode) & _S_IFDIR)
#endif

#ifndef myint64
#ifdef _WIN32
typedef __int64			myint64;
#else
typedef long long       myint64;
#endif
#endif

//! 대용량 파일 정보를 획득하기 위한 클래스
class CStat64 {
#ifdef _WIN32
	struct _stati64* m_pStat;
#elif _OSF1
	struct stat m_stat;
#elif _USE_CLIB
	struct stat m_stat;
#elif _SOLARIS == 5
	struct stat m_stat;
#elif _FREEBSD
	struct stat m_stat;
#else
	struct stat64 m_stat;
#endif
	int			m_ok;

public:
	CStat64();
	~CStat64();

	//! 입력한 경로의 파일상태정보를 가져온다. Return; success(0), fail(otherwise)
	int	Stat(char* pPath);	
	//! 입력한 경로의 파일상태정보를 가져온다. (like lstat(char *path)) Return; success(0), fail(otherwise)
	int	LStat(char* pPath);

	//! Stat 함수 사용이후 stat.st_mode값을 리턴한다.
	unsigned int 	GetMode();
	//! Stat 함수 사용이후 stat.st_uid값을 리턴한다.
	unsigned int	GetUid();
	//! Stat 함수 사용이후 stat.st_gid값을 리턴한다.
	unsigned int	GetGid();
	//! Stat 함수 사용이후 stat.st_size값을 리턴한다.
	myint64			GetSize();
	//! Stat 함수 사용이후 stat.st_atime값을 리턴한다.
	unsigned int	GetATime();
	//! Stat 함수 사용이후 stat.st_mtime값을 리턴한다.
	unsigned int	GetMTime();	
	//! Stat 함수 사용이후 stat.st_ctime값을 리턴한다.
	unsigned int	GetCTime();

	//! 디렉토리인지를 검사하고 디렉토리라면 1을 리턴하고 아니면 0을 리턴한다.
	int	IsFold();
	//! 파일인지를 검사하고 디렉토리라면 1을 리턴하고 아니면 0을 리턴한다.
	int	IsFile();
	//! 링크인지를 검사하고 디렉토리라면 1을 리턴하고 아니면 0을 리턴한다.
	int	IsLink();

	//! Stat 함수 사용후 stat.st_mtime값을 변경한다.
	static int SetMTime(char* pPath,unsigned int uTime);
	//! Stat 함수 사용후 stat.st_mtime값을 변경한다.
	static int SetMTime(char* pPath,time_t tTime);
	//! 디렉토리를 생성한다.
	static int MkDir(char* pPath,int nMode=0700);
};

#endif
