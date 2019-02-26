/*
   Copyright (c) 2003 BrainzSquare, Inc.
   file64.cpp - file 64 bit i/o operation

   2005.06.10. added ReadLine() by hjson
   2005.06.23. modified ReadLine(), _File64GetToken() by hjson
   2007.07.20. modified Open(), by kskim
 */

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcpsock.h"
#include "file64.h"

CFile64::CFile64()
{
#ifdef _WIN32
	m_hFile = INVALID_HANDLE_VALUE;
#else
	m_fp = NULL;
#endif
	m_nReadTot = 0;
}

CFile64::~CFile64()
{
	Close();
}

int CFile64::Open(char* pPath,char* pMode,int nShare)
{
#ifdef _WIN32
	// 2005.12.01.
	DWORD dwShare = 0;

	// modified by kskim. 2007-07-20
	// dwShare |= FILE64_SHARE_READ  => dwShare |= FILE_SHARE_READ
	// dwShare |= FILE64_SHARE_WRITE  => dwShare |= FILE_SHARE_WRITE

	if(nShare & FILE64_SHARE_READ) dwShare |= FILE_SHARE_READ;
	if(nShare & FILE64_SHARE_WRITE) dwShare |= FILE_SHARE_WRITE;
	//
	if(!strncmp(pMode,"rb",2)) {
		m_hFile = CreateFile(pPath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_SEQUENTIAL_SCAN,
			NULL);
	} else if(!strncmp(pMode,"wb",2)) {
		m_hFile = CreateFile(pPath,
			GENERIC_WRITE,
			dwShare,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	} else {
		m_hFile = CreateFile(pPath,
			GENERIC_WRITE,
			dwShare,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	}
	return (m_hFile != INVALID_HANDLE_VALUE ? 1 : 0);
#elif _OSF1
	m_fp = fopen(pPath,pMode);
	return(m_fp ? 1:0);
#elif _NETWARE
	m_fp = fopen(pPath,pMode);
	return(m_fp ? 1:0);
#elif _SOLARIS == 5
	m_fp = fopen(pPath,pMode);
	return(m_fp ? 1:0);
#elif _FREEBSD
	m_fp = fopen(pPath,pMode);
	return(m_fp ? 1:0);
#else
	m_fp = fopen64(pPath,pMode);
	return (m_fp ? 1 : 0);
#endif
}

myint64 CFile64::Seek(myint64 offset,int origin)
{
#ifdef _WIN32
	if(m_hFile == INVALID_HANDLE_VALUE) return -1;

	DWORD dwFlag = FILE_BEGIN;
	LONG lLow, lHigh;
	UINT low, high;
	switch(origin) {
	case SEEK_SET: dwFlag = FILE_BEGIN; break;
	case SEEK_CUR: dwFlag = FILE_CURRENT; break;
	case SEEK_END: dwFlag = FILE_END; break;
	}
	// int64 -> ui + ui
	low = (unsigned int)(offset & 0xffffffff);
	high = (unsigned int)(offset >> 32);
	lLow = (LONG)low; lHigh = (LONG)high;
	lLow = SetFilePointer(m_hFile,lLow,&lHigh,dwFlag);
	low = (UINT)lLow; high = (UINT)lHigh;
	// ui + ui -> int64
	offset = high; offset = offset << 32; offset |= low;
#elif _OSF1
	if(m_fp == NULL) return -1;
	offset = fseek(m_fp,offset,origin);
#elif _SOLARIS == 5
	if(m_fp == NULL) return -1;
	offset = fseek(m_fp,offset,origin);
#elif _FREEBSD
	if(m_fp == NULL) return -1;
	offset = fseek(m_fp,offset,origin);
#elif _USE_CLIB
	if(m_fp == NULL) return -1;
	offset = fseek(m_fp,offset,origin);
#else
	if(m_fp == NULL) return -1;
	offset = fseeko64(m_fp,offset,origin);
#endif
	return offset;
}

int CFile64::Write(char* val,int size)
{
#ifdef _WIN32
	DWORD nwritten = 0;
	if(m_hFile!=INVALID_HANDLE_VALUE)
		WriteFile(m_hFile,val,size,&nwritten,NULL);
#else
	int nwritten = 0;
	if(m_fp) nwritten = fwrite(val,1,size,m_fp);
#endif
	return nwritten;
}

int CFile64::Read(char* val,int size)
{
#ifdef _WIN32
	DWORD nread = 0;
	if(m_hFile!=INVALID_HANDLE_VALUE)
		ReadFile(m_hFile,val,size,&nread,NULL);
#else
	int nread = 0;
	if(m_fp) nread = fread(val,1,size,m_fp);
#endif
	return nread;
}

static int _File64GetToken(char* pData,int& nLen,char* pTok,int nSize)
{
    if(nLen == 0) return 0;

    int i, nNum, nOkay = 0;

    // tokenize
    nNum = 0;
    for(i=0;i<nLen;i++) {
		if(nNum >= nSize-1) break;
        pTok[nNum++] = pData[i];
		if(pTok[nNum-1] == '\n') {
			nOkay = 1;
			break;
		}
    }
    pTok[nNum] = '\0';

    // move last string to first
    if(nOkay || nNum+1>=nSize) {
		nLen = nLen - nNum;
		memcpy(pData,&pData[nNum],nLen);
        pData[nLen] = '\0';
    } else {
	    nNum = 0;
        pTok[nNum] = '\0';
    }

    return nNum;
}

int CFile64::ReadLine(char* val,int size)
{
	int nRead = 0, nLen = 0;

	do {
		nRead = _File64GetToken(m_szReadBuf,m_nReadTot,val,size);
		if(nRead > 0) break;	// read line okay

		nLen = Read(&m_szReadBuf[m_nReadTot],FILE64_MAX-m_nReadTot-1);
		if(nLen <= 0) break;	// no more data

		m_nReadTot += nLen;
	} while(1);

	return nRead;
}

void CFile64::Close()
{
#ifdef _WIN32
	if(m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
#else
	if(m_fp) {
		fclose(m_fp);
		m_fp = NULL;
	}
#endif
	m_nReadTot = 0;
}

#ifdef _FILE64_TEST

int main(int argc,char* argv[])
{
	if(argc != 2) {
		printf("Usage: %s <filename>\n",argv[0]);
		return 0;
	}

	CFile64 f64;
	char buf[1024];

	// SEEK 이용할 경우, 반드시 w+b 이용할 것!! 2007.07.12. by hjson
	if(f64.Open(argv[1],"w+b")) {
		strcpy(buf,"hjson");
		f64.Seek(210974,SEEK_SET);
		f64.Write(buf,strlen(buf));
		f64.Close();
	}

	if(f64.Open(argv[1],"rb")) {
		int line = 0;
		while(f64.ReadLine(buf,sizeof(buf)) > 0) {
			printf("Line %d: %s",++line,buf);
		}
		f64.Close();
	}

	return 0;
}

#endif
