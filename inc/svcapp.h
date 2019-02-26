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

//! Zenius Application을 재작하기 위한 Framework class 로서 어플리케이션의 Entrypoint역활과 Exit 역활을 한다. 주된기능은 Unix, Linux, Window 마다의 서로 다른 어플리케이션에서 공통된 Framework를 제공한다.
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
	/** Windows에서 서비스에 추가가 된다. */
	virtual void 	DoInstall(int idx, int argc, char *argv[]);
	/** Windows에서 서비스에서 삭제한다. */
	virtual void 	DoUninstall(int idx, int argc, char *argv[]);
	/** 프로그램의 시작함수로서 반드시 재정의가 내려져야 한다. */
	virtual void 	DoStart(int idx, int argc, char *argv[]) { }
	/** 프로그램의 자동업그레이드를 위한 함수로서 재정의가 필요없다. */
	virtual void 	DoUpgrade(int idx, int argc, char *argv[]);
	/** 프로그램의 help메세지를 위한 함수로서 재정의가 내려져야 한다. */
	virtual void 	DoHelpMsg(int idx, int argc, char *argv[]) {}
	/** 프로그램의 version메세지를 위한 함수로서 재정의가 내려져야 한다. */
	virtual void 	DoVersionMsg(int idx, int argc, char *argv[]) {}

	virtual void	SetCurDir(char *svcname,char *pszCurDir);
	/** 프로그램시작전에 호출되는 함수로서 재정의 하여 사용될수 있다.*/
	virtual void	PreProcessCommand(int argc, char *argv[]) 	{}
	/** 프로그램종료후에 호출되는 함수로서 재정의 하여 사용될수 있다.*/
	virtual void	PostProcessCommand(int argc, char *argv[])	{}

	// added by kskim.(req. hjson) 2007.2.27
	//! Child Process가 시작된 시간을 지정한다.
	void	SetRestartTime(int nSec = 30); // child process restart time

	// variables
public:
	char	m_szIniName[128];		//! 환경변수 파일이름 (확장자 포함)
	char	m_szLogName[128];		//! 로그 파일이름 (확장자 포함)
	char	m_szSvcName[128];		//! 서비스(데몬) 이름
	char 	m_szProgFileName[128];	//! 확장자 포함
	char 	m_szProgramName[128];	//! 확장자 미포함
	char	m_szProgramPath[512];	//! 프로그램 경로(current dir,ex)/var/wizard)
	char	m_szExecPath[512];		//! 실행파일실행 경로 (ex, /usr/sbin)
	char	m_szWatchDogFile[128];	//! 왓치독 파일 이름 (확장자 포함)
	char	m_szUpgrade[128];		//! 업그레이드 파일 이름 (확장자 포함)
	char    m_szUpgradeNew[128];    //! 업그레이드를 위해 사용하는 임시파일
	char	m_szPid[128];			//! PID(s) 파일 이름 (확장자 포함)
	int		m_bUseWatchDog; 		//! 왓치독 사용여부 
	// added by kskim. 2008.07.21
	char	m_szExecFullName[512];	//! 경로 + 이름 + 확장자
	char	m_szEnvPath[512];		//! PROGRAM_PATH 값
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
//! 최대 로그 사이즈(bytes)를 결정 
int SetLogSize(int nMaxSize = 5000000); 
//! 디폴트 log파일에 기록하는 함수
void Log(const char *pszMsg, ...); 
//! 지정한 Log파일에 기록하는 함수
void LogFile(char *pszFileName, const char *, ... );
//! 프로그램 이름으로 된 INI파일에 문자정보를 기록하는 함수
void WriteIniString(char *pszItem, char *pszValue, char* pComment=NULL);
//! 프로그램 이름으로 된 INI파일에 Integer를 기록하는 함수
void WriteIniInt(char *pszItem, int nValue, char* pComment=NULL);
//! 프로그램 이름으로 된 INI파일에서 문자정보를 읽는 함수
int  ReadIniString(char *pszItem,char *pszVal,int nSize,char *pszDef=NULL);
//! 프로그램 이름으로 된 INI파일에서 Integer정보를 읽는 함수
int  ReadIniInt(char *pszItem, int &nVal, int nDef=0);
//! 프로그램이 시작될때 Debug모드로 시작되었는가를 판단하는 변수
//! -debug 옵션으로 시작될때 1로 설정됨
extern int	g_nSvcAppDebugLvl;
//! 프로그램이 사용한 ZEN.lib, libzen-*.a의 버전을 리턴한다.
char* GetZenLibVersion();

#endif
