// mydefs.cc

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

#ifdef	_WIN32
#include <windows.h>
#include <io.h>
#else
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "mydefs.h"

int mysystem(char* path, unsigned int nTimeout /*milliseconds*/)
{
#ifdef _WIN32
	int nRet = -1;
	DWORD dwWait;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// 2006.04.~ by kskim
	if(path == NULL) return nRet;

	memset(&si,0,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	memset (&pi,0,sizeof(pi));

	BOOL bRet = CreateProcess(NULL, path,
		NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
		NULL, NULL, &si, &pi);
	if(bRet) {
		nRet = 1;
		if(nTimeout) { // 2006.04.10 by kskim
			dwWait = ::WaitForSingleObject(pi.hProcess,nTimeout);
			if(dwWait == WAIT_TIMEOUT) nRet = 0;
		}
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}
	return nRet;
#else
	if (path == NULL) return 1;

	// 2003.11.06. by hjson
	int status = !system(path);
	return status;
#endif
}

int SetLimit(int nType,int nLimit)
{
	int ret = -1;
	char status[32], name[32];
	struct rlimit rl;

	strcpy(status,"-");
#if (_DEBIAN==3 || _RH==78) // add rh78 kimjh 2010.07.28
	if(getrlimit((__rlimit_resource_t)nType,&rl) == 0)
#else
	if(getrlimit(nType,&rl) == 0) 
#endif
	{
		rl.rlim_cur = nLimit;
#if (_DEBIAN==3 || _RH==78) // add rh78 kimjh 2010.07.28
		ret = setrlimit((__rlimit_resource_t)nType,&rl);
#else
		ret = setrlimit(nType,&rl);
#endif
		if(ret == 0) strcpy(status,"OK");
		else strcpy(status,"Fail");
	}
	switch(nType) {
	case RLIMIT_RSS: strcpy(name,"Set Mem Limit"); break;
	case RLIMIT_NOFILE: strcpy(name,"Set FD Limit"); break;
	case RLIMIT_CORE: strcpy(name,"Set Core Limit"); break;
	default: strcpy(name,"Unknown Limit"); break;
	}
	fprintf(stderr,"%s (%d)\n",name,nLimit);

	return ret;
}
