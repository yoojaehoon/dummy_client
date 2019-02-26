/*
   Copyright (c) 2003-2005 BrainzSquare, Inc.
   stat64.cpp - stat 64 bit operation
 */

#include "stdafx.h"
#include "stat64.h"

#ifdef _WIN32
#include <sys/utime.h>
#include <direct.h>
#else
#include <utime.h>
#endif

CStat64::CStat64()
{
#ifdef _WIN32
	m_pStat = new struct _stati64;
	memset(m_pStat,0,sizeof(struct _stati64));
#else
	memset(&m_stat,0,sizeof(m_stat));
#endif
	m_ok = 0;
}

CStat64::~CStat64()
{
#ifdef _WIN32
	if(m_pStat) delete m_pStat;
#endif
}

int CStat64::Stat(char* pPath)
{
	int ret = 0;
#ifdef _WIN32
	ret = _stati64(pPath,m_pStat);
#elif _OSF1
	ret = stat(pPath,&m_stat);
#elif _SOLARIS == 5
	ret = stat(pPath,&m_stat);
#elif _FREEBSD
	ret = stat(pPath,&m_stat);
#elif _USE_CLIB
	ret = stat(pPath,&m_stat);
#else
	ret = stat64(pPath,&m_stat);
#endif
	if(ret == 0) m_ok = 1;
	else m_ok = 0;
	return ret;
}

int CStat64::LStat(char* pPath)
{
	int ret = 0;
#ifdef _WIN32
	ret = _stati64(pPath,m_pStat);
#elif _OSF1
	ret = lstat(pPath,&m_stat);
#elif _SOLARIS == 5
	ret = lstat(pPath,&m_stat);
#elif _FREEBSD
	ret = lstat(pPath,&m_stat);
#elif _USE_CLIB
	ret = stat(pPath,&m_stat);
#elif _USE_LIBC
	ret = stat64(pPath,&m_stat);
#else
	ret = lstat64(pPath,&m_stat);
#endif
	if(ret == 0) m_ok = 1;
	else m_ok = 0;
	return ret;
}


unsigned int CStat64::GetMode()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return m_pStat->st_mode;
#else
	return m_stat.st_mode;
#endif
}

unsigned int CStat64::GetUid()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return m_pStat->st_uid;
#else
	return m_stat.st_uid;
#endif
}

unsigned int CStat64::GetGid()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return m_pStat->st_gid;
#else
	return m_stat.st_gid;
#endif
}

myint64 CStat64::GetSize()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return m_pStat->st_size;
#else
	return m_stat.st_size;
#endif
}

unsigned int CStat64::GetATime()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return m_pStat->st_atime;
#elif defined (_NETWARE) && defined(_USE_LIBC)
	return m_stat.st_atime.tv_sec;
#else
	return m_stat.st_atime;
#endif
}

unsigned int CStat64::GetMTime()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return m_pStat->st_mtime;
#elif defined (_NETWARE) && defined(_USE_LIBC)
	return m_stat.st_mtime.tv_sec;
#else
	return m_stat.st_mtime;
#endif
}

unsigned int CStat64::GetCTime()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return m_pStat->st_ctime;
#elif defined (_NETWARE) && defined(_USE_LIBC)
	return m_stat.st_ctime.tv_sec;
#else
	return m_stat.st_ctime;
#endif
}

int CStat64::IsFold()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return S_ISDIR(m_pStat->st_mode);
#else
	return S_ISDIR(m_stat.st_mode);
#endif
}

int CStat64::IsFile()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return S_ISREG(m_pStat->st_mode);
#else
	return S_ISREG(m_stat.st_mode);
#endif
}

int CStat64::IsLink()
{
	if(!m_ok) return 0;
#ifdef _WIN32
	return S_ISLNK(m_pStat->st_mode);
#else
	return S_ISLNK(m_stat.st_mode);
#endif
}

int CStat64::SetMTime(char* pPath,unsigned int uTime)
{
	return SetMTime(pPath,(time_t)uTime);
}

int CStat64::SetMTime(char* pPath,time_t tTime)
{
	int ret = 0;
#ifdef _WIN32
	struct _utimbuf ub;
#else
	struct utimbuf ub;
#endif
	ub.actime = tTime;
	ub.modtime = tTime;
#ifdef _WIN32
	if(_utime(pPath,&ub) == 0) ret = 1;
#else
	if(utime(pPath,&ub) == 0) ret = 1;
#endif
	return ret;
}

int CStat64::MkDir(char* pPath,int nMode)
{
	int ret = 0;
#ifdef _WIN32
	if(_mkdir(pPath) == 0) ret = 1;
#else
	if(mkdir(pPath,nMode) == 0) ret = 1;
#endif
	return ret;
}

#ifdef _STAT64_TEST

int main(int argc,char* argv[])
{
	if(argc != 2) {
		printf("Usage: %s <filename>\n",argv[0]);
		return 0;
	}

	CStat64 st;
	if(st.Stat(argv[1]) == 0) printf("GetMTime %u\n",st.GetMTime());
	else printf("Failed %u\n",st.GetMTime());
	return 0;
}

#endif
