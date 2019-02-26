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
//! �Է��� ���μ��� �̸��� ���Ե� ��� ���μ����� ���� �����Ų��.
int		killall(const char* pname, int signum);
//! �Է��� ���μ��� ID�� ���� �����Ų��.
int		killpid(int pid,int signum);
/** �Է��� ���μ����̸����� �����ϴ� ���μ����߿��� 
�ι�°�� �Է��� PID���� ���� ���μ����� ���翩�θ� �Ǵ��Ѵ�. 
�����Ѵٸ� 1�̻��� ���� ���ϵǰ� �ƴ϶�� 0�� ���ϵȴ�.
*/
int		killchk(const char* pname,int pid);

#endif	// _KILLALL_H
