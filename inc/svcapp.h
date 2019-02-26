/*
   Copyright (c) 2002 BrainzSquare, Inc.
   svcapp.cpp - main framework

History:
	2009.1.6 by kskim
		- summary: at DoInstall, be used iostream functions instead of fgets
*/

#include <time.h>	// time_t

#ifndef __SVCAPP_H__
#define __SVCAPP_H__

#ifdef _WIN32
#include "exceptrpt.h"
#include <Userenv.h>
#include <WtsApi32.h>
#include <Tlhelp32.h>
#endif

// modified by kskim 2008.9.9
#define INI_USEWATCHDOG     "USE_WATCHDOG"
#define INI_SVCNAME         "SVC_NAME"

//
#define WATCHDOG_UPGRADE    "UPGRADE"
#define WATCHDOG_RESTART    "RESTART" 	// added by kskim. 2009.4.8

class CSvcApp;

typedef void (CSvcApp::*CMDPROC)(int, int, char *[]);

typedef struct tagCMDMAP
{
	char	szCmd[32];
	CMDPROC	proc;
} CMDMAP, *PCMDMAP;

#define DECLARE_CMDMAP() \
protected: \
static const CMDMAP _cmdEntry[]; \
virtual const CMDMAP* _GetCmdEntry() { return _cmdEntry; } \
virtual PCMDMAP _GetCmdMap(char *cmd); \
public:

#define BEGIN_CMDMAP(theClass) \
PCMDMAP theClass::_GetCmdMap(char *cmd) { \
	int i = 0;								\
	while(_cmdEntry[i].proc) { \
		if(strlen(_cmdEntry[i].szCmd) > 0) { \
			if(!strncmp(_cmdEntry[i].szCmd,cmd,sizeof(_cmdEntry[i].szCmd)))\
			return (PCMDMAP) &(_cmdEntry[i]); \
		} \
		i++; \
	} \
	return (PCMDMAP)NULL; \
} \
const CMDMAP theClass::_cmdEntry[] = { \
	ON_CMDHANDLER("-install", CSvcApp::DoInstall) \
	ON_CMDHANDLER("-start", CSvcApp::DoService) \
	ON_CMDHANDLER("-run", CSvcApp::DoRunService) \
	ON_CMDHANDLER("-debug", CSvcApp::DoDebug) \
	ON_CMDHANDLER("-stop", CSvcApp::DoStop) \
	ON_CMDHANDLER("-kill", CSvcApp::DoKill) \
	ON_CMDHANDLER("-uninstall", CSvcApp::DoUninstall) \
	ON_CMDHANDLER("-upgrade", CSvcApp::DoUpgrade) \
	ON_CMDHANDLER("-show", CSvcApp::DoShow) \
	ON_CMDHANDLER("-i", CSvcApp::DoViewIni) \
	ON_CMDHANDLER("-h", theClass::DoHelpMsg) \
	ON_CMDHANDLER("-v", theClass::DoVersionMsg) 

#define END_CMDMAP() { "", NULL } };

#define ON_CMDHANDLER(cmd, proc) {cmd, (CMDPROC) &proc},

#ifdef WIN32
class CEzServiceStatus : public SERVICE_STATUS
{
public:
	SERVICE_STATUS_HANDLE m_hStatus;

public:
					CEzServiceStatus();
	virtual			~CEzServiceStatus();
	inline void		InitStatus(SERVICE_STATUS_HANDLE hStatus)
					{ m_hStatus = hStatus; }
	BOOL			SetState(DWORD dwState, DWORD aCheckPoint = -1, DWORD aWaitHint = -1);
	inline DWORD	GetState() { return dwCurrentState; }

	// jhson
	bool			IsAnyRDPSessionActive();
	void			DisconnectRemoteSessions();
};
#endif

//! Zenius Application�� �����ϱ� ���� Framework class �μ� ���ø����̼��� Entrypoint��Ȱ�� Exit ��Ȱ�� �Ѵ�. �ֵȱ���� Unix, Linux, Window ������ ���� �ٸ� ���ø����̼ǿ��� ����� Framework�� �����Ѵ�.
class CSvcApp
{
protected:
	virtual PCMDMAP _GetCmdMap(char* cmd) { return NULL; } 
	// functions
public:
	CSvcApp();
	virtual ~CSvcApp();

	int		ProcessCommand(int argc, char *argv[]);
	void 	WatchDogSet(char *pszTitle);
	time_t 	WatchDogGet(char *pszTitle);
	int		IsCont()		{ return !m_bStop;		}
	int		GetChildPid()	{ return m_nChildPid;	}	// 2005.03.18.
	int		Upgrade(char* pszFileName);				// 2005.04.18. by kskim

	// return >= 0, if success	2005.08.30.
	int		GetOsDesc(char* pOsDesc, int nOsDesc);
	
	// attribute
private:
	void	Signal();
	int		IsExistProgram();
	void	KillProgram();
	void	WritePid(int ppid, int cpid);
	void	GetProgramProperty(char *arg);
	int 	DispatchCommand(int idx, int argc, char **argv);
	void 	DispatchCommand(PCMDMAP pMap, int idx, int argc, char **argv);
	void 	WatchDog();
	int		Upgrading(int pid=-1);			// 2005.04.18. by kskim
#ifdef WIN32
	// update by jhson 2008.07.28
	static void WINAPI ServiceMain(DWORD dwArgs, LPTSTR* pszArgv);
	static DWORD WINAPI ServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
#endif
	void 	SetProgramPath(char *arg); // 2008.07.21 added by kskm

public:
	void 	DoService(int idx, int argc, char *argv[]);
	void 	DoDebug(int idx, int argc, char *argv[]);
	void 	DoRunService(int idx, int argc, char *argv[]);
	void 	DoStop(int idx, int argc, char *argv[]);
	void 	DoKill(int idx, int argc, char *argv[]);
	void 	DoShow(int idx, int argc, char *argv[]);
	void	DoViewIni(int idx, int argc, char *argv[]);
	void	DoLocService(int idx, int argc, char *argv[]); // added by kskim.2006.10.23
	/** Stop: override for stopping application(s) */
	virtual void 	Stop(int nSig=0) { m_bStop = 1; }
	/** OnSigUsr: override for using SIGUSR1(10), SIGUSR2(12) */
	virtual void	OnSigUsr(int nSig); 	// 2008.04.28 by hjson

//protected:
	/** Windows���� ���񽺿� �߰��� �ȴ�. */
	virtual void 	DoInstall(int idx, int argc, char *argv[]);
	/** Windows���� ���񽺿��� �����Ѵ�. */
	virtual void 	DoUninstall(int idx, int argc, char *argv[]);
	/** ���α׷��� �����Լ��μ� �ݵ�� �����ǰ� �������� �Ѵ�. */
	virtual void 	DoStart(int idx, int argc, char *argv[]) { }
	/** ���α׷��� �ڵ����׷��̵带 ���� �Լ��μ� �����ǰ� �ʿ����. */
	virtual void 	DoUpgrade(int idx, int argc, char *argv[]);
	/** ���α׷��� help�޼����� ���� �Լ��μ� �����ǰ� �������� �Ѵ�. */
	virtual void 	DoHelpMsg(int idx, int argc, char *argv[]) {}
	/** ���α׷��� version�޼����� ���� �Լ��μ� �����ǰ� �������� �Ѵ�. */
	virtual void 	DoVersionMsg(int idx, int argc, char *argv[]) {}

	virtual void	SetCurDir(char *svcname,char *pszCurDir);
	/** ���α׷��������� ȣ��Ǵ� �Լ��μ� ������ �Ͽ� ���ɼ� �ִ�.*/
	virtual void	PreProcessCommand(int argc, char *argv[]) 	{}
	/** ���α׷������Ŀ� ȣ��Ǵ� �Լ��μ� ������ �Ͽ� ���ɼ� �ִ�.*/
	virtual void	PostProcessCommand(int argc, char *argv[])	{}

	// added by kskim.(req. hjson) 2007.2.27
	//! Child Process�� ���۵� �ð��� �����Ѵ�.
	void	SetRestartTime(int nSec = 30); // child process restart time

	// variables
public:
	char	m_szIniName[128];		//! ȯ�溯�� �����̸� (Ȯ���� ����)
	char	m_szLogName[128];		//! �α� �����̸� (Ȯ���� ����)
	char	m_szSvcName[128];		//! ����(����) �̸�
	char 	m_szProgFileName[128];	//! Ȯ���� ����
	char 	m_szProgramName[128];	//! Ȯ���� ������
	char	m_szProgramPath[512];	//! ���α׷� ���(current dir,ex)/var/wizard)
	char	m_szExecPath[512];		//! �������Ͻ��� ��� (ex, /usr/sbin)
	char	m_szWatchDogFile[128];	//! ��ġ�� ���� �̸� (Ȯ���� ����)
	char	m_szUpgrade[128];		//! ���׷��̵� ���� �̸� (Ȯ���� ����)
	char    m_szUpgradeNew[128];    //! ���׷��̵带 ���� ����ϴ� �ӽ�����
	char	m_szPid[128];			//! PID(s) ���� �̸� (Ȯ���� ����)
	int		m_bUseWatchDog; 		//! ��ġ�� ��뿩�� 
	// added by kskim. 2008.07.21
	char	m_szExecFullName[512];	//! ��� + �̸� + Ȯ����
	char	m_szEnvPath[512];		//! PROGRAM_PATH ��
#ifdef WIN32
	HANDLE	m_hEvent; 		// user defined event. added by kskim.2008.8.25
#endif

private:


protected:
	int		m_bStop;
	time_t	m_tStart, m_tWatchDog;
	int		m_nChildPid;			// child process id
	// added by kskim 2007.2.27
	int 	m_nRestartTm; 			// child restart time(sec)
	// added by kskim 2008.07.21
	int 	m_nExeUid; 				// execution user (unix, linux)
	int 	m_nOwnerUid; 			// file owner (unix, linux)
	int 	m_nGroupUid; 			// file group id of owner (unix, linux)
#ifdef WIN32
	HANDLE	m_hStop;
	DWORD	m_dwLastControlStatus;
	CEzServiceStatus m_status;
	SERVICE_STATUS_HANDLE	m_hStatus;
#endif
};

// added by kskim 2007.2.27
//! �ִ� �α� ������(bytes)�� ���� 
int SetLogSize(int nMaxSize = 5000000); 
//! ����Ʈ log���Ͽ� ����ϴ� �Լ�
void Log(const char *pszMsg, ...); 
//! ������ Log���Ͽ� ����ϴ� �Լ�
void LogFile(char *pszFileName, const char *, ... );
//! ���α׷� �̸����� �� INI���Ͽ� ���������� ����ϴ� �Լ�
void WriteIniString(char *pszItem, char *pszValue, char* pComment=NULL);
//! ���α׷� �̸����� �� INI���Ͽ� Integer�� ����ϴ� �Լ�
void WriteIniInt(char *pszItem, int nValue, char* pComment=NULL);
//! ���α׷� �̸����� �� INI���Ͽ��� ���������� �д� �Լ�
int  ReadIniString(char *pszItem,char *pszVal,int nSize,char *pszDef=NULL);
//! ���α׷� �̸����� �� INI���Ͽ��� Integer������ �д� �Լ�
int  ReadIniInt(char *pszItem, int &nVal, int nDef=0);
//! ���α׷��� ���۵ɶ� Debug���� ���۵Ǿ��°��� �Ǵ��ϴ� ����
//! -debug �ɼ����� ���۵ɶ� 1�� ������
extern int	g_nSvcAppDebugLvl;
//! ���α׷��� ����� ZEN.lib, libzen-*.a�� ������ �����Ѵ�.
char* GetZenLibVersion();

#endif
