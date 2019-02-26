/*
   Copyright (c) 2003 BrainzSquare, Inc.
   fold64.h - stat 64bit operation
 */

#ifndef __FOLD64_H__
#define __FOLD64_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
// nothing to include
#else
#include <dirent.h>
#endif

//! 디렉토리를 찾기위한 클래스
class CFold64 {
#ifdef _WIN32
	HANDLE	m_hFind;
	BOOL	m_bNext;
	WIN32_FIND_DATA	m_wfd;
#else
	DIR*	m_pDir;
	struct dirent *m_next;
#endif

public:
	CFold64();
	~CFold64();

	// Open() : return TRUE if success
	//! 찾고자 하는 디렉토리의 상위 디렉토리를 Open하기 위한 함수 */
	int		Open(char* pPath);
	// Read() : return TRUE if success
	//! Open한 디렉토리로 부터 원하는 디렉토리를 찾는 함수로서 성공하면 1을 리턴한다. */
	int		Read(char* pBuf,int nBuf);
	// Close()
	//! Open한 디렉토리를 닫는 함수로서 메모리 해제를 위해 반드시 호출되어야 한다. */
	void	Close();
};

/*
	CFold64 fold;
    char buf[256];
	if(fold.Open("/tmp")) {
		while(fold.Read(buf,sizeof(buf))) {
			printf("%s\n",buf);
		}
		fold.Close();
	}
*/

#endif
