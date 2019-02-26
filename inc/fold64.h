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

//! ���丮�� ã������ Ŭ����
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
	//! ã���� �ϴ� ���丮�� ���� ���丮�� Open�ϱ� ���� �Լ� */
	int		Open(char* pPath);
	// Read() : return TRUE if success
	//! Open�� ���丮�� ���� ���ϴ� ���丮�� ã�� �Լ��μ� �����ϸ� 1�� �����Ѵ�. */
	int		Read(char* pBuf,int nBuf);
	// Close()
	//! Open�� ���丮�� �ݴ� �Լ��μ� �޸� ������ ���� �ݵ�� ȣ��Ǿ�� �Ѵ�. */
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
