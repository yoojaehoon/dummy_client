/*
 * Copyright (c) 2003-2005 BrainzSquare, Inc.
 * fold64.cpp - fold 64 bit operation
 *
 */

#include "stdafx.h"
#include "fold64.h"

CFold64::CFold64()
{
#ifdef _WIN32
	m_hFind = INVALID_HANDLE_VALUE;
	m_bNext = FALSE;
#else
	m_pDir = NULL;
	m_next = NULL;
#endif
}

CFold64::~CFold64()
{
	Close();
}

int CFold64::Open(char* pPath)
{
	int ret = 0;
#ifdef _WIN32
    // concatnate *.*
    char file[256];
    _snprintf(file,sizeof(file),"%s/*.*",pPath);
    m_hFind = FindFirstFile(file,&m_wfd);
    if(m_hFind != INVALID_HANDLE_VALUE) {
		m_bNext = TRUE;
		ret = 1;
	}
#else
	m_pDir = opendir(pPath);
	if(m_pDir) ret = 1;
#endif
	return ret;
}

int CFold64::Read(char* pBuf,int nBuf)
{
	int ret = 0;
#ifdef _WIN32
    if(m_hFind != INVALID_HANDLE_VALUE && m_bNext) {
		ret = 1;
		strncpy(pBuf,m_wfd.cFileName,nBuf-1);
       	m_bNext = FindNextFile(m_hFind,&m_wfd);
    }
#else
    if(m_pDir && (m_next=readdir(m_pDir))) {
		ret = 1;
		strncpy(pBuf,m_next->d_name,nBuf-1);
    }
#endif
	return ret;
}

void CFold64::Close()
{
#ifdef _WIN32
	if(m_hFind != INVALID_HANDLE_VALUE) {
		FindClose(m_hFind);
		m_hFind = INVALID_HANDLE_VALUE;
		m_bNext = FALSE;
	}
#else
	if(m_pDir) {
		closedir(m_pDir);
		m_pDir = NULL;
	}
#endif
}

#ifdef _FOLD64_TEST

int main(int argc,char* argv[])
{
	CFold64 fold;
	char buf[256];

	if(argc != 2) {
		printf("Usage: %s <directory>\n",argv[0]);
		return 0;
	}

	if(fold.Open(argv[1])) {
		while(fold.Read(buf,sizeof(buf))) {
			printf("%s\n",buf);
		}
		fold.Close();
	}

	return 0;
}

#endif
