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

//! ��뷮 ���� ������ ȹ���ϱ� ���� Ŭ����
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

	//! �Է��� ����� ���ϻ��������� �����´�. Return; success(0), fail(otherwise)
	int	Stat(char* pPath);	
	//! �Է��� ����� ���ϻ��������� �����´�. (like lstat(char *path)) Return; success(0), fail(otherwise)
	int	LStat(char* pPath);

	//! Stat �Լ� ������� stat.st_mode���� �����Ѵ�.
	unsigned int 	GetMode();
	//! Stat �Լ� ������� stat.st_uid���� �����Ѵ�.
	unsigned int	GetUid();
	//! Stat �Լ� ������� stat.st_gid���� �����Ѵ�.
	unsigned int	GetGid();
	//! Stat �Լ� ������� stat.st_size���� �����Ѵ�.
	myint64			GetSize();
	//! Stat �Լ� ������� stat.st_atime���� �����Ѵ�.
	unsigned int	GetATime();
	//! Stat �Լ� ������� stat.st_mtime���� �����Ѵ�.
	unsigned int	GetMTime();	
	//! Stat �Լ� ������� stat.st_ctime���� �����Ѵ�.
	unsigned int	GetCTime();

	//! ���丮������ �˻��ϰ� ���丮��� 1�� �����ϰ� �ƴϸ� 0�� �����Ѵ�.
	int	IsFold();
	//! ���������� �˻��ϰ� ���丮��� 1�� �����ϰ� �ƴϸ� 0�� �����Ѵ�.
	int	IsFile();
	//! ��ũ������ �˻��ϰ� ���丮��� 1�� �����ϰ� �ƴϸ� 0�� �����Ѵ�.
	int	IsLink();

	//! Stat �Լ� ����� stat.st_mtime���� �����Ѵ�.
	static int SetMTime(char* pPath,unsigned int uTime);
	//! Stat �Լ� ����� stat.st_mtime���� �����Ѵ�.
	static int SetMTime(char* pPath,time_t tTime);
	//! ���丮�� �����Ѵ�.
	static int MkDir(char* pPath,int nMode=0700);
};

#endif
