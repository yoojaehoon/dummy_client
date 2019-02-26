/*
 * killall.h - kill processes by name
 * Copyright (C) 2004 Brainzsquare, Inc.
 *
 * PLATFORM: WIN32/LINUX/SOLARIS/AIX/HPUX/FREEBSD/UNIXWARE/OSF1/IRIX
 * 2004/05/16 <changmin@brainz.co.kr>
 * 2005/01/27 <kskim@brainz.co.kr> add window's killall
 */

#ifndef _KILLALL_H
#define _KILLALL_H

#ifdef _AIX 
//#if	(_AIX < 61)	// added by kskim. 2010.3.3
extern "C" {
	int getprocs(void	*ProcessBuffer,
			int				ProcessSize,
			void *FileBuffer,
			int				FileSize,
			pid_t				*IndexPointer,
			int				Count);
}
//#endif
#endif

#if TEST
int		killmain(const char* pname, int argc, char *argv[]);
#endif
//! 입력한 프로세스 이름이 포함된 모든 프로세스를 강제 종료시킨다.
int		killall(const char* pname, int signum);
//! 입력한 프로세스 ID를 강제 종료시킨다.
int		killpid(int pid,int signum);
/** 입력한 프로세스이름으로 시작하는 프로세스중에서 
두번째로 입력한 PID값을 같는 프로세스가 존재여부를 판단한다. 
존재한다면 1이상의 값이 리턴되고 아니라면 0이 리턴된다.
*/
int		killchk(const char* pname,int pid);

#endif	// _KILLALL_H
