/*
 * killall.cc - kill processes by name
 * Copyright (C) 2004 Brainzsquare, Inc.
 *
 * PLATFORM: WIN32/LINUX/SOLARIS/AIX/HPUX/UNIXWARE/FREEBSD/OSF1/IRIX
 * 2004/05/16 <changmin@brainz.co.kr>
 * 2005/01/27 <kskim@brainz.co.kr> add window's killall
 */

extern "C" {
#ifndef _WIN32
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#endif

#if	(_FREEBSD)
#include <fcntl.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#elif	(_AIX)
#include <procinfo.h>
#include <sys/proc.h>

#elif	(_OSF1)
#include <sys/proc.h>
#include <sys/table.h>
#include <mach.h>
#include <mach/mach_types.h>
#include <mach/vm_statistics.h>
#include <sys/syscall.h>

#elif	(_HPUX)
#include <sys/param.h>
#include <sys/pstat.h>

#elif	(_SOLARIS || _UNIXWARE || _IRIX)
#include <sys/procfs.h>
#endif

}
#ifdef _WIN32
#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#endif

#include "killall.h"

static int killprocs(const char *proc_name, int signum);
static int get_proc_count(const char *pname, int pid);

int killpid(int pid, int signum)
{
	int ret = -1;
#ifdef _WIN32
	HANDLE	hProc;
	hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if(hProc) {
		TerminateProcess(hProc, signum);
		CloseHandle(hProc);
		ret = 0;
	}
#else
	ret = kill(pid, signum);
#endif
	return ret;
}

int killall(const char *name, int signum)
{
#ifndef _WIN32
    if (killprocs(name, signum) < 0)
		return -1;
#else
	if (killprocs(name, signum) == 0)
		return -1;
#endif
	
    return 0;
}

int killchk(const char* pname,int pid)
{
	int exist = 0;

#ifdef _WIN32
	char pname2[512], pproc[512];
	int i, len, curpid = -1;

	len = strlen(pname);
	for(i=0; i<len; i++) pname2[i] = tolower(pname[i]);
	pname2[i] = '\0';

	HMODULE	hModule;
	HANDLE hProc;
	DWORD dwNeed;
	curpid = (int) GetCurrentProcessId();
	if(curpid == pid || curpid == -1) return exist;

	hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if(hProc) {
		if(::EnumProcessModules(hProc,&hModule,sizeof(hModule),&dwNeed)) {
			::GetModuleBaseName(hProc,hModule,pproc,sizeof(pproc));
			len = strlen(pproc);
			for(i=0;i<len;i++) pproc[i] = tolower(pproc[i]);
			if(strstr(pproc, pname2)) exist = 1;
		}
		CloseHandle(hProc);
	}
#else
	if(pid == getpid()) exist = 0;
	else {
		exist = get_proc_count(pname,pid);
		if(exist < 0) exist = 0;
	}
#endif

	return exist;
}

#if		(_LINUX || _SOLARIS || _UNIXWARE || _IRIX)
int killprocs(const char *proc_name, int signum)
{
    struct dirent	*ent		= NULL;
    DIR				*proc_dir	= NULL;
    int				pid			= -1;
    int				cur_pid		= -1;
	int				cur_ppid	= -1;
    int				killed_pid	= -1;
    int				proc_count	= 0;
#ifndef _KSKIM
	// 디렉토리에서 중간에 없어진 디렉토리가 있으면 링크가 깨져서 
	// 못가져오는 경우가 발생함. (rh78에서 발생한적이 있음)
	// 죽여야할 pid를 전부 가져온다음에 삭제하게 수정함. by kskim. 2008.10.8
#define MAX_PID 1024
	// added by kskim
	int 			pids[MAX_PID];
	int 			cnt = 0;
	memset(pids, 0, sizeof(int)*MAX_PID);
#endif

    if (!proc_name)
		return -1;
    
    if (signum < 0)
		killed_pid = 0;
    
    proc_dir = opendir("/proc");
    if (!proc_dir)
		return -1;

    cur_pid = getpid();
	cur_ppid = getppid();
    while ((ent = readdir(proc_dir))) {
		pid = atoi(ent->d_name);
		if (pid < 1 || pid == cur_pid || pid == cur_ppid)
			continue;
	
		proc_count = get_proc_count(proc_name, pid);
		if (proc_count < 0) {
			continue;
		}
	
		if (signum < 0)
			killed_pid += proc_count;
		else {
			killed_pid = pid;
#ifdef _KSKIM
			kill(pid, signum);
			//printf("killed pid[%d], signum[%d]\n", killed_pid, signum);
#else
			if(cnt<MAX_PID)
				pids[cnt++] = pid;
			else {
				for(int i=0; i<cnt; i++) {
					if(pids[i]>0) {
						kill(pids[i], signum);
						//printf("killed pid[%d], signum[%d]\n",pids[i], signum);
						pids[i] = 0;
					}
				}
				kill(pid, signum);
				//printf("killed pid[%d], signum[%d]\n", killed_pid, signum);
				cnt = 0;
			}
#endif
		}
    }
#ifndef _KSKIM
	for(int i=0; i<cnt; i++) {
		if(pids[i]>0) {
			kill(pids[i], signum);
			//printf("killed pid[%d][%d], signum[%d]\n",i,pids[i], signum);
		}
	}
#endif
    closedir(proc_dir);
    
    return killed_pid;
}

#elif	(_FREEBSD)
int killprocs(const char *proc_name, int signum)
{
    kvm_t				*kvmp		= NULL;
    struct kinfo_proc	*kpp		= NULL;
    int					nproc		= 0;
    int					cur_pid		= -1;
	int					cur_ppid	= -1;
    pid_t				killed_pid	= -1;
    int					i			= 0;

    if (!proc_name)
		return -1;
    
    if (signum < 0)
		killed_pid = 0;
    
    kvmp = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open");
    if (!kvmp)
		return -1;
    
    kpp = kvm_getprocs(kvmp, KERN_PROC_ALL, 0, &nproc);
    if (!kpp)
		return -1;

    cur_pid = getpid();
	cur_ppid = getppid();
    for (i = 0; kpp && i < nproc; i++, kpp++) {
#if		_FREEBSD >= 50
		if (kpp->ki_pid != cur_pid
			&& kpp->ki_pid != cur_ppid
			&& strstr(kpp->ki_comm, proc_name)) {
		    if (signum < 0)
				killed_pid++;
		    else {
				killed_pid = kpp->ki_pid;
				kill(killed_pid, signum);
				//printf("killed pid[%d], signum[%d]\n", killed_pid, signum);
		    }
		}
#else
		if (kpp->kp_proc.p_pid != cur_pid
			&& kpp->kp_proc.p_pid != cur_ppid
			&& strstr(kpp->kp_proc.p_comm, proc_name)) {
		    if (signum < 0)
				killed_pid++;
		    else {
				killed_pid = kpp->kp_proc.p_pid;
				kill(killed_pid, signum);
				//printf("killed pid[%d], signum[%d]\n", killed_pid, signum);
		    }
		}
#endif	// _FREEBSD >= 50
    }
    kvm_close(kvmp);
    
    return killed_pid;
}

#elif	(_AIX)
int killprocs(const char *proc_name, int signum)
{
	struct procsinfo	pinfo[128];
	int					psize		= sizeof(struct procsinfo);
	pid_t				pindex		= 0;
    int					cur_pid		= -1;
	int					cur_ppid	= -1;
    int					killed_pid	= -1;
	int					maxcnt		= 128;
	int					i, num;

    if (!proc_name) return -1;
    
    cur_pid = getpid();
	cur_ppid = getppid();
    if (signum < 0) killed_pid = 0;
	while ((num=getprocs(pinfo,psize,NULL,0,&pindex,maxcnt)) > 0) {
		for(i=0;i<num;i++) {
			if (pinfo[i].pi_pid != cur_pid
				&& pinfo[i].pi_pid != cur_ppid
				&& strstr(pinfo[i].pi_comm, proc_name))
			{
				if (signum < 0)
					killed_pid += pinfo[i].pi_thcount;
				else {
					killed_pid = pinfo[i].pi_pid;
					kill(killed_pid, signum);
					//printf("killed pid[%d], signum[%d]\n", killed_pid, signum);
				}
			}
		}
    }

    return killed_pid;
}

#elif	(_HPUX)
int killprocs(const char *proc_name, int signum)
{
    struct pst_dynamic	pdnm;
    struct pst_static	p_static;
    struct pst_status	*p_status;

	int cur_pid = getpid();
	int cur_ppid = getppid();
	int killed_pid = signum < 0 ? 0 : -1;

    if (!proc_name)
		return -1;
    
    if (signum < 0)
		killed_pid = 0;

    if (pstat_getstatic(&p_static, sizeof(struct pst_static), 1, 0) < 0)
		return -1;

    if (pstat_getdynamic(&pdnm, sizeof(struct pst_dynamic), 1, 0) < 0)
		return -1;

    int nprocs   = pdnm.psd_activeprocs + 1;
	int pidx = 0;
	for (int idx = 0; idx < nprocs; idx++) {
		struct pst_status pinfo;
		int count = pstat_getproc(&pinfo, sizeof(pinfo), 1, pidx++);
		pidx = pinfo.pst_idx +1;
		if (pinfo.pst_pid != cur_pid
				&& pinfo.pst_ucomm
				&& strstr(pinfo.pst_ucomm, proc_name)) {
			if (signum < 0)
#if             (_HPUX10_20)
				killed_pid += 1;
#else
			killed_pid += pinfo.pst_nlwps;
#endif
			else {
				killed_pid = (pid_t) pinfo.pst_pid;
				kill(killed_pid, signum);
				//printf("killed pid[%d], signum[%d]\n",
				//	killed_pid, signum);
			}
		}
	}

    return killed_pid;
}

#elif	(_OSF1)
int killprocs(const char *proc_name, int signum)
{
    struct tbl_procinfo	p_i[8];
    int					nproc		= 0;
    pid_t				cur_pid		= -1;
	pid_t				cur_ppid	= -1;
    pid_t				killed_pid	= -1;
    int					j			= 0;
    int					k			= 0;
    int					r			= 0;
    
    if (!proc_name)
		return -1;
    
    if (signum < 0)
		killed_pid = 0;
    
    nproc = table(TBL_PROCINFO, 0, (struct tbl_procinfo *) NULL, INT_MAX, 0);
    if (nproc < 1)
		return -1;
    
    cur_pid		= getpid();
	cur_ppid	= getppid();
    for (j = 0; j < nproc; j += 8) {
		memset(p_i, 0, sizeof p_i);
		r = table(TBL_PROCINFO, j, (struct tbl_procinfo *) p_i, 8,
				  sizeof(struct tbl_procinfo));
		for (k = 0; k < r; k++) {
			if (p_i[k].pi_pid != cur_pid
				&& p_i[k].pi_pid != cur_ppid
				&& strstr(p_i[k].pi_comm, proc_name)) {
				if (signum < 0)
					killed_pid++;
				else {
					killed_pid = p_i[k].pi_pid;
					kill(killed_pid, signum);
					//printf("killed pid[%d], signum[%d]\n", killed_pid, signum);
				}
			}
		}
    }
    
    return killed_pid;
}
#elif	(_WIN32)
int killprocs(const char *proc_name, int signum)
{
	int	nRes=0; 
	unsigned int i=0, j=0, len;
	unsigned int uProc; // proc array count
	unsigned int uCurPid;
	DWORD	dwNeed, dwProcArr[2048];
	HMODULE	hModule;
	HANDLE	hProcess;
	char szProc[MAX_PATH+1], szFind[MAX_PATH+1], szSrc[MAX_PATH+1];

	len = strlen(proc_name);
	for(j=0, i=0; i<len; i++, j++) szFind[j] = tolower(proc_name[i]);
	szFind[i] = '\0';

	// get proc list
	memset(dwProcArr, 0, sizeof(dwProcArr));
	::EnumProcesses(dwProcArr,sizeof(dwProcArr),&dwNeed);
	uProc=dwNeed/sizeof(DWORD);

	// cur pid
	uCurPid = (unsigned int)GetCurrentProcessId();

	for(i=0;i<uProc;i++) {
		if(uCurPid != dwProcArr[i]) {
			hProcess=OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwProcArr[i]);
			if (hProcess) {
				// 첫번째 모듈(=프로세스 그 자체)의 이름을 구해 출력한다.
				if(::EnumProcessModules(hProcess,&hModule,
							sizeof(hModule),&dwNeed)) {
					::GetModuleBaseName(hProcess,hModule,szProc,
							sizeof(szProc));
					len = strlen(szProc);
					for(j=0;j<len;j++) szSrc[j] = tolower(szProc[j]);
					szSrc[j] = '\0';
					if(strstr(szFind, szSrc)) {
						if(TerminateProcess(hProcess, 0))
							nRes++;
					}
				}
				CloseHandle(hProcess);
			}
		}
	}
	return nRes;
}
#endif

#if		(_LINUX)
static int get_proc_count(const char *name, int pid)
{
    char	stat_path[256];
    char	proc_name[256];
    FILE	*stat_fp		 = NULL;
    int		ret_pid			 = -1;

    if (pid < 1 || !name || strlen(name) < 1)
		return -1;
    
    snprintf(stat_path, sizeof stat_path - 1, "/proc/%d/stat", pid);
    stat_fp = fopen(stat_path, "r");
    if (!stat_fp)
		return -1;
    
    fscanf(stat_fp, "%d (%[^)]) %*s", &ret_pid, proc_name);
    fclose(stat_fp);

    if (strncmp(proc_name, name, sizeof proc_name - 1) || pid != ret_pid)
		return -1;
    
    return 1;
}

#elif	(_SOLARIS)
static int get_proc_count(const char *name, int pid)
{
    int			stat_fd;
    char		stat_path[256];
    prpsinfo_t	psinfo;
    prstatus_t	prstatus;
    
    if (!name || pid < 1 || strlen(name) < 1)
		return -1;

    memset(&psinfo, 0, sizeof psinfo);
    memset(&prstatus, 0, sizeof prstatus);
	
    sprintf(stat_path, "/proc/%d", pid);
    stat_fd = open(stat_path, O_RDONLY);
    if (stat_fd < 0)
		return -1;
    
    if (ioctl(stat_fd, PIOCPSINFO, &psinfo) < 0
		|| ioctl(stat_fd, PIOCSTATUS, &prstatus) < 0) {
		close(stat_fd);
		return -1;	
    }
    close(stat_fd);
    
    if (strncmp(name, psinfo.pr_fname, strlen(name))
		|| pid != psinfo.pr_pid)
		return -1;
    
    return prstatus.pr_nlwp;
}

#elif	(_UNIXWARE)
static int get_proc_count(const char *name, int pid)
{
    int			stat_fd;
    char		stat_path[256];
    psinfo_t	psinfo;
    
    if (!name || pid < 1 || strlen(name) < 1)
		return -1;
    
    memset(&psinfo, 0, sizeof psinfo);
    snprintf(stat_path, sizeof stat_path - 1, "/proc/%d/psinfo", pid);
    stat_fd = open(stat_path, O_RDONLY);
    if (stat_fd < 0)
		return -1;
    
    read(stat_fd, &psinfo, sizeof psinfo);
    close(stat_fd);
    
    if (strncmp(name, psinfo.pr_fname, strlen(name))
		|| pid != psinfo.pr_pid)
		return -1;
    
    return psinfo.pr_nlwp;
}

#elif	(_IRIX)
static int get_proc_count(const char *name, int pid)
{
    int			stat_fd;
    char		stat_path[256];
    prpsinfo	psinfo;
    
    if (!name || pid < 1 || strlen(name) < 1)
		return -1;

    memset(&psinfo, 0, sizeof psinfo);
    
    sprintf(stat_path, "/proc/%d", pid);
    stat_fd = open(stat_path, O_RDONLY);
    if (stat_fd < 0)
		return -1;
    
    if (ioctl(stat_fd, PIOCPSINFO, &psinfo) < 0) {
		close(stat_fd);
		return -1;	
    }
    close(stat_fd);
    
    if (strncmp(name, psinfo.pr_fname, strlen(name))
		|| pid != psinfo.pr_pid)
		return -1;

    return 1;
}

#elif	(_FREEBSD)
static int get_proc_count(const char *pname, int pid)
{
    kvm_t				*kvmp		= NULL;
    struct kinfo_proc	*kpp		= NULL;
    int					nproc		= 0;
    int					cur_pid		= -1;
	int					cur_ppid	= -1;
    pid_t				ret			= -1;
    int					i			= 0;

    if (!pname || !pid) return ret;
    
    kvmp = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open");
    if (!kvmp) return ret;
    
    kpp = kvm_getprocs(kvmp, KERN_PROC_ALL, 0, &nproc);
    if (!kpp) {
    	kvm_close(kvmp);
		return ret;
	}

    cur_pid = getpid();
	cur_ppid = getppid();
    for (i = 0; kpp && i < nproc; i++, kpp++) {
#if		_FREEBSD >= 50
		if (kpp->ki_pid != cur_pid
			&& kpp->ki_pid != cur_ppid
			&& kpp->ki_pid == pid
			&& strstr(kpp->ki_comm, pname))
		{
			ret = kpp->ki_pid;
			break;
		}
#else
		if (kpp->kp_proc.p_pid != cur_pid
			&& kpp->kp_proc.p_pid != cur_ppid
			&& kpp->kp_proc.p_pid == pid
			&& strstr(kpp->kp_proc.p_comm, pname))
		{
			ret = 1;
			break;
		}
#endif	// _FREEBSD >= 50
    }
    kvm_close(kvmp);
    
    return ret;
}

#elif	(_AIX)
static int get_proc_count(const char *pname, int pid)
{
	struct procsinfo	pinfo[128];
	int	psize = sizeof(struct procsinfo);
	pid_t pindex		= 0;
    pid_t cur_pid	= -1;
    int cur_ppid	= -1;
    int ret			= -1;
	int maxcnt		= 128;
	int i, num;

    if (!pname || !pid) return ret;
    
    cur_pid = getpid();
    cur_ppid = getppid();
    while ((num=getprocs(pinfo,psize,NULL,0,&pindex,maxcnt)) > 0) {
		for(i=0;i<num;i++) {
			if ((int)pinfo[i].pi_pid != cur_pid
				&& (int)pinfo[i].pi_pid != cur_ppid
				&& (int)pinfo[i].pi_pid == pid
				&& strstr(pinfo[i].pi_comm, pname))
			{
				ret = 1;
				break;
			}
		}
    }

    return ret;
}

#elif	(_HPUX)
static int get_proc_count(const char *pname, int pid)
{
	struct pst_dynamic  pdnm;
    struct pst_static	p_static;
    struct pst_status	*p_status;
    struct pst_status   *p_status_cur;
    int					pst_size	= 0;
    int					nprocs		= 0;
    int					idx			= 0;
    int					count		= 0;
    pid_t				cur_pid		= -1;
	pid_t				cur_ppid	= -1;
    pid_t				ret			= -1;
    
    if (!pname || !pid) return ret;
    
    if (pstat_getstatic(&p_static, sizeof(struct pst_static), 1, 0) < 0)
		return ret;

	if (pstat_getdynamic(&pdnm, sizeof(struct pst_dynamic), 1, 0) < 0)
        return -1;

    nprocs = pdnm.psd_activeprocs + 1;
    pst_size = p_static.pst_status_size * nprocs;
    p_status = (struct pst_status *) malloc(pst_size);
    if (!p_status) return ret;
    
    cur_pid = getpid();
	cur_ppid = getppid();
    memset(p_status, 0, pst_size);
    count = pstat_getproc(p_status, sizeof(struct pst_status), nprocs, 0);
    for (idx = 0; idx < count; idx++) {
		p_status_cur = p_status + idx;
		if (p_status_cur->pst_pid != cur_pid
			&& p_status_cur->pst_pid != cur_ppid
			&& p_status_cur->pst_ucomm
			&& p_status_cur->pst_pid == pid
			&& strstr(p_status_cur->pst_ucomm, pname))
		{
			ret = 1;
			break;
		}
    }
    free(p_status);

    return ret;
}

#elif	(_OSF1)
static int get_proc_count(const char *pname, int pid)
{
    struct tbl_procinfo	p_i[8];
    int					nproc		= 0;
    pid_t				cur_pid		= -1;
	pid_t				cur_ppid	= -1;
    pid_t				ret			= -1;
    int					j			= 0;
    int					k			= 0;
    int					r			= 0;
    
    if (!pname || !pid) return ret;
    
    nproc = table(TBL_PROCINFO, 0, (struct tbl_procinfo *) NULL, INT_MAX, 0);
    if (nproc < 1) return ret;
    
    cur_pid		= getpid();
	cur_ppid	= getppid();
    for (j = 0; j < nproc; j += 8) {
		memset(p_i, 0, sizeof p_i);
		r = table(TBL_PROCINFO, j, (struct tbl_procinfo *) p_i, 8,
				  sizeof(struct tbl_procinfo));
		for (k = 0; k < r; k++) {
			if (p_i[k].pi_pid != cur_pid
				&& p_i[k].pi_pid != cur_ppid
				&& p_i[k].pi_pid == pid
				&& strstr(p_i[k].pi_comm, pname))
			{
				ret = 1;
				break;
			}
		}
    }
    
    return ret;
}

#endif

#ifdef TEST
int killmain(const char *name, int argc, char *argv[])
{
	if (!name || !strlen(name))
		return -1;

	if (argc > 1) {
		if (!strncmp(argv[1], "stop", 5) || !strncmp(argv[1], "-stop", 6)) {
			if (KillAll(name, SIGTERM) < 0)
				fprintf(stderr, "%s: failed to stop the %s\n", name, name);
			exit(0);
		}
		if (!strncmp(argv[1], "kill", 5) || !strncmp(argv[1], "-kill", 6)) {
			if (KillAll(name, SIGKILL) < 0)
				fprintf(stderr, "%s: failed to kill the %s\n", name, name);
			exit(0);
		}
	}

	return 0;
}
#endif

