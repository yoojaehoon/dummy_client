/*
   Copyright (c) 2002 BrainzSquare, Inc.
   svcapp.cpp - main framework

History:
	2010.11.05 by kskim
		- summary: 2010년 수정요청사항 반영, IPv6반영을 위한 정기 업데이트
	2009.10.13 by kskim
		- summary: DoStop, DoKill 로그 추가
		- 사용자가 시그널을 날린것을 로그로 남김
	2009.9.11 by kskim
		- summary: modified DoInstall function
		- 등록되지 않은 사용자로 install할경우 중단되게 수정함

	2009.8.21 by kskim
		- summary: modified WatchDog function
		- '-stop' 사용해서 정상 종료시 Parent Process가 DoStart를 실행하는 문제를 수정함.
		- 동시에 두개의 프로세스가 DoStart를 실행하면서 프로그램 처음 Write INI 하는 부분에서
		  INI 내용이 동시에 file write가 이뤄지면서 INI 파일이 깨지는 현상 발견됨.
		- summary: modified WriteIniString function
		- fopen(... , r) 일경우에 모든 파일 쓰기가 이뤄지도록 수정함
	2009.7.28 by kskim
		- summary: modified Num2IP function
		- because inet_ntoa failed, has instead of  inet_ntop
	2009.5.22 by kskim
		- summary: modified DoUpgrade function
		- in case user mode, could not file m_szExecPath
	2009.4.13 by kskim
		- summary: added WATCHDOG_RESTART
		- In _WIN32 case, WatchDog function had modified 
	2009.4.10 by kskim
		- summary: added WATCHDOG_RESTART
		- WatchDog function, modified wait(...) to waitpid(...)
	2009.1.6 by kskim
		- summary: at DoInstall, be used iostream functions instead of fgets
*/
#include "stdafx.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#include <time.h>
#include <psapi.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <fcntl.h>
#endif
#include <fcntl.h>

#include "killall.h"
#include "svcapp.h"
#include "snprintf.h"
#include "mutext.h"
#include "thread.h"
#ifndef _WIN32
#include "rcscript.h" // added by kskim. 2008.7.16
#include "which.h" // added by kskim. 2008.9.2
#endif

#include <list>

#ifndef mysleep
#ifdef _WIN32
#define mysleep(n)  Sleep(1000*(n))
#else
#define mysleep(n)  sleep(n)
#endif
#endif

#ifdef _USE_IPV4ONLY
#define ZENLIB_VER 	"2010.12.10 (IPv4 Only)"
#else
#define ZENLIB_VER 	"2010.12.10"
#endif

struct _IniItem {
	char line[256];
	char comment[256];
};

static	CSvcApp* g_pApp = NULL;
static  CMutext  g_mutIni;
int 	g_nSvcAppDebugLvl = 0;
// 최대로그 사이즈
int		g_nMaxLog = 5000000;

#ifndef WIN32
typedef void (*SaHandler) (int);
void SigAction(int sig, SaHandler sah)
{
	struct sigaction act;
	memset(&act, 0, sizeof act);
	act.sa_handler = sah;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, sig);
	sigaction(sig, &act, NULL);
}
#else
//
typedef DWORD (*WTSGETACTIVECONSOLESESSIONID)();
WTSGETACTIVECONSOLESESSIONID g_lpfnWTSGetActiveConsoleSessionId = NULL;
#endif
// 
char* GetZenLibVersion() { return (char*) ZENLIB_VER; }
// 
int SetLogSize(int nMaxSize)
{
	if(nMaxSize >= 5000000) {
		g_nMaxLog = nMaxSize;
		return 1;
	}
	return 0;
}

#ifdef _20100427_KSKIM
// AIX 4.3버전에서 kernel문제 있음으로 피해가기 위해 변경함.
static void _Log(char* pFile, char *buf)
{
	static CMutext _mut;
	FILE* fp = NULL;
	struct stat fs;
	char backup[256];

	_mut.Lock();
	if(stat(pFile,&fs) == 0 && fs.st_size >= g_nMaxLog) {
		mysnprintf(backup,255,"%s.1",pFile);
		unlink(backup);
		rename(pFile,backup);
	}
	fp = fopen(pFile,"a");
    if(fp) {
#ifndef _WIN32
		if(g_pApp &&  getuid()>0) {
			chown(pFile, getuid(), getgid());
		}
#endif
        char timebuf[256];
       	time_t ttime = time(NULL);
       	strcpy(timebuf,ctime(&ttime));
	    timebuf[strlen(timebuf)-1] = '\0';
	    fprintf(fp,"%s %s",timebuf,buf);
        fclose(fp);
    }
	_mut.Unlock();
}
#else
static void _Log(char* pFile, char *buf)
{
	static CMutext _mut;
	struct stat fs;
	char backup[256];
	int fd;

	_mut.Lock();
	if(stat(pFile,&fs) == 0 && fs.st_size >= 5000000) {
		mysnprintf(backup,255,"%s.1",pFile);
		unlink(backup);
		rename(pFile,backup);
	}
#if defined (_MSC_VER) && (_MSC_VER >= 1020) // visual 2008 의 경우
	fd = _open(pFile, _O_CREAT | _O_WRONLY | _O_APPEND, _S_IREAD | _S_IWRITE); // 2010.05.04 hjson
#else
	fd = open(pFile,O_CREAT|O_WRONLY|O_APPEND,0644); // 2010.05.04 hjson
#endif
	if(fd > 0) {
#ifndef _WIN32
		if(getuid()>0) {
			chown(pFile, getuid(), getgid());
		}
#endif
		// make time
		char timebuf[256];
		time_t ttime = time(NULL);
		strcpy(timebuf,ctime(&ttime));
		timebuf[strlen(timebuf)-1] = '\0';
		strcat(timebuf," ");
		// write time
		write(fd,timebuf,strlen(timebuf));
		// write data
		write(fd,buf,strlen(buf));
		// close fd
		close(fd);
	}
	_mut.Unlock();
}
#endif

void Log(const char *pszMsg, ... )
{
	if(!g_pApp) return;

	char szBuf[1024];

	memset(szBuf, 0, sizeof(szBuf));
	va_list args; 
	va_start(args, pszMsg); 
#ifdef _WIN32
	myvsnprintf(szBuf, sizeof(szBuf)-1, pszMsg, args ); 
#elif (_AIX || _HPUX)
	myvsnprintf(szBuf, sizeof(szBuf)-1, pszMsg, args ); 
#else
	vsnprintf(szBuf, sizeof(szBuf)-1, pszMsg, args ); 
#endif
	va_end(args); 
	szBuf[strlen(szBuf)] = '\0';
	
	_Log(g_pApp->m_szLogName, szBuf);
}

void LogFile(char *pszFileName, const char *pszMsg, ... )
{
	char szBuf[1024];

	memset(szBuf, 0, sizeof(szBuf));
	va_list args; 
	va_start(args, pszMsg); 
	myvsnprintf(szBuf, sizeof(szBuf)-1, pszMsg, args ); 
	va_end(args); 
	szBuf[strlen(szBuf)] = '\0';
	
	_Log(pszFileName, szBuf);
}

void WriteIniString(char *pszItem, char *pszVal, char *pComment)
{
	FILE* fp;
	int okay = 0;
	char *filename = g_pApp->m_szIniName;
	char line[256];
	int linelen = sizeof(line);

	std::list<_IniItem> qItem;
	_IniItem ii;

	// read
	g_mutIni.Lock(); // 2005.12.8 by kskim
	fp = fopen(filename,"r");
	if(fp) {
		memset(line, 0, sizeof(line));
		memset(&ii, 0, sizeof(ii));
		while(fgets(line,linelen-1,fp)) {
			char item[256], comment[256];
			memset(item, 0, sizeof(item));
			memset(comment, 0, sizeof(comment));
			// comment (# by user, ## by program)
			if(line[0] == '#') {
				if(line[1] == '#') {
					strcpy(ii.comment, line);
				}else {
					strcpy(ii.line,line);
					qItem.push_back(ii);
				}
			}
			else {
				// item
#ifdef _WIN32
				int nd = sscanf(line,"%[^ =] = %*[^\0\n ]",item);
#else
				int nd = sscanf(line,"%[^ =] = %*[^\n ]",item);
#endif
				if(nd==1) {
					if((!strcmp(item, pszItem))) { // 기존의 존재값
						okay = 1;
						if(pComment && strlen(pComment) > 0) {
							mysnprintf(ii.comment,linelen,"## %s\n",pComment);
						}
						mysnprintf(ii.line,linelen,"%s = %s\n",pszItem,pszVal);
						qItem.push_back(ii);
					} else {
						memset(ii.line, 0, linelen);
						strcpy(ii.line, line);
						qItem.push_back(ii);
					}
					memset(&ii, 0, sizeof(ii));
				}
			}
		}
		fclose(fp);
	}

	// if not exist, then return
	if(!okay) {
		memset(&ii, 0, sizeof(ii));
		if(pComment && strlen(pComment)>0) {
			mysnprintf(ii.comment,linelen,"## %s\n",pComment);
		}
		mysnprintf(ii.line,linelen,"%s = %s\n",pszItem,pszVal);
		qItem.push_back(ii);
	}

	// write
	fp = fopen(filename,"w");
	if(fp) {
		std::list<_IniItem>::iterator it;
		it = qItem.begin();
		while(it != qItem.end()) {
			if(strlen(it->comment)>0) {
				fprintf(fp,"%s",it->comment);
			}
			if(strlen(it->line)>0) {
				fprintf(fp,"%s",it->line);
			}
			++it;
		}
		fclose(fp);
	}

	g_mutIni.Unlock(); // 2005.12.8 by kskim
}

void WriteIniInt(char *pszItem, int nValue, char *pComment)
{
	char szVal[256];
	mysnprintf(szVal,sizeof(szVal),"%d",nValue);
	WriteIniString(pszItem,szVal,pComment);
}

int ReadIniString(char *pszItem, char *pszVal, int nSize, char *pszDef)
{
	FILE* fp;
	int ok = 0;
	char item[256];
	char szOri[256];

	// initialize
	memset(pszVal, 0, nSize);
	memset(szOri, 0, sizeof(szOri));

	//
	g_mutIni.Lock(); // 2005.12.8 by kskim
	fp = fopen(g_pApp->m_szIniName,"r");
	if(fp) {
		char buf[256];
		while(fgets(buf,255,fp)) {
			if(buf[0] == '#') continue;
#ifdef _WIN32
			int nd = sscanf(buf,"%[^ =] = %[^\n]",item, szOri);
#else
			int nd = sscanf(buf,"%[^ =] = %[^\n]",item, szOri);
#endif
			if(nd==2) {
				if((!strcmp(item, pszItem))) { 
					ok =1;
					break;
				}
			}
		}
		fclose(fp);
	}
	if(ok) strncpy(pszVal, szOri, nSize);
	else if(!ok && pszDef) strncpy(pszVal, pszDef, nSize);

	g_mutIni.Unlock(); // 2005.12.8 by kskim

	return ok;
}

int ReadIniInt(char *pszItem, int &nValue, int nDef)
{
	int ok = 0;
	char szDef[256],szVal[256];
	mysnprintf(szDef,sizeof(szDef),"%d",nDef);
	ok = ReadIniString(pszItem,szVal,sizeof(szVal),szDef);
	nValue = atoi(szVal);
	return ok;
}

#ifdef _WIN32

THREADTYPE WaitEventThread(LPVOID pParam)
{
	while(g_pApp->IsCont()) {
		if(WaitForSingleObject(g_pApp->m_hEvent,(DWORD)3000)==WAIT_OBJECT_0)
		{
			g_pApp->OnSigUsr(0);
		}
	}
	return 0;
}

int OSVersionAndPermission(void)
{
	OSVERSIONINFO osvi;
	int bIsWinNT = 0;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	
	if(GetVersionEx(&osvi)==FALSE) {
		fprintf(stderr, "Can't determine Windows version");
		return 0;
	}
	if(osvi.dwPlatformId==VER_PLATFORM_WIN32s) {
		fprintf(stderr, "Maybe, not worked at _WIN32s");
		return 0;
	}
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) bIsWinNT = 1;
	else bIsWinNT = 0;

	if(bIsWinNT) {
		HANDLE tok;
		if(OpenProcessToken(GetCurrentProcess(),
					TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&tok)) {
			LUID luid;
			TOKEN_PRIVILEGES tp;

			LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&luid);
			tp.PrivilegeCount=1;
			tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			tp.Privileges[0].Luid=luid;
			AdjustTokenPrivileges(tok,FALSE,&tp,NULL,NULL,NULL);
		
			LookupPrivilegeValue(NULL,SE_SECURITY_NAME,&luid);
			tp.PrivilegeCount=1;
			tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			tp.Privileges[0].Luid=luid;
			AdjustTokenPrivileges(tok,FALSE,&tp,NULL,NULL,NULL);

			LookupPrivilegeValue(NULL,SE_TCB_NAME,&luid);
			tp.PrivilegeCount=1;
			tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			tp.Privileges[0].Luid=luid;
			AdjustTokenPrivileges(tok,FALSE,&tp,NULL,NULL,NULL);
			//
			// Enable the SE_DEBUG_NAME privilege
			//
			LUID DebugValue;
			if (LookupPrivilegeValue((LPSTR) NULL,SE_DEBUG_NAME,&DebugValue)) {
				tp.PrivilegeCount = 1;
				tp.Privileges[0].Luid = DebugValue;
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				AdjustTokenPrivileges(tok,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),
					(PTOKEN_PRIVILEGES) NULL,(PDWORD) NULL);
			}
			CloseHandle(tok);
		}
	}

	return bIsWinNT;
}

// NT high version
DWORD GetWinLogonPID()
{
	//DWORD dwSessionId = 0;
	DWORD dwExplorerLogonPid = 0;
	PROCESSENTRY32 procEntry;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hSnap == INVALID_HANDLE_VALUE) return 0;

	procEntry.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hSnap, &procEntry)) {
		CloseHandle(hSnap);
		return 0;
	}

	do
	{
		if(_stricmp(procEntry.szExeFile, "winlogon.exe") == 0) {
			dwExplorerLogonPid = procEntry.th32ProcessID;
		}

	} while(Process32Next(hSnap, &procEntry));

	CloseHandle(hSnap);

	return dwExplorerLogonPid;
}

// NT low version
DWORD FindWinLogon(DWORD SessionId)
{
	PWTS_PROCESS_INFO pProcessInfo = NULL;
	DWORD			  ProcessCount = 0;	
	DWORD			  Id = 0;

	BOOL			  bResult = FALSE;

	if(WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &ProcessCount))
	{
		// dump each process description
		for(DWORD CurrentProcess = 0; CurrentProcess < ProcessCount; CurrentProcess++)
		{
			if(_stricmp(pProcessInfo[CurrentProcess].pProcessName, "winlogon.exe") == 0)
			{
				if(SessionId == pProcessInfo[CurrentProcess].SessionId)
				{
					Id = pProcessInfo[CurrentProcess].ProcessId;
					bResult = TRUE;
					break;
				}
			}
		}
		WTSFreeMemory(pProcessInfo);
	} else {
		Log("Failed to WTSEnumerateProcesses session id(%d)\n", SessionId);
	}

	Log("FindWinLogon SessionId(%d) ID(%d) Result(%d)\n", SessionId, Id, bResult);

	return Id;
}
BOOL GetSessionUserToken(OUT LPHANDLE  lphUserToken)
{
	BOOL   bResult  = FALSE;
	HANDLE hProcess = NULL;
	HANDLE hAccessToken = NULL;
	HANDLE hTokenThis = NULL;
	DWORD  dwSessionID = 0;
	
	if(g_lpfnWTSGetActiveConsoleSessionId != NULL) 
		dwSessionID = g_lpfnWTSGetActiveConsoleSessionId();
	//DWORD Id = FindWinLogon(dwSessionID);
	DWORD Id = GetWinLogonPID();

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id);
	if(hProcess) {
		bResult = OpenProcessToken(hProcess, 
						//TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS,  // TOKEN_IMPERSONATE
						TOKEN_ADJUST_PRIVILEGES|TOKEN_ADJUST_SESSIONID|TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS,  // TOKEN_IMPERSONATE
						&hTokenThis);

		if(bResult) {
			bResult = DuplicateTokenEx(hTokenThis, 
					TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, // TOKEN_IMPERSONATE
					NULL, 
					SecurityImpersonation, 
					TokenPrimary, 
					lphUserToken);
			if(bResult) {
				SetTokenInformation(*lphUserToken, TokenSessionId, &dwSessionID, sizeof(DWORD));
			} else {
				Log("Failed to DuplicateTokenEx GetLastError(%d)\n", GetLastError());
			}
		} else {
			Log("Failed to OpenProcessToken GetLastError(%d)\n", GetLastError());
		}

		CloseHandle(hTokenThis);		
		CloseHandle(hProcess);
	} else {
		Log("Failed to open as active console session id(%d)\n", Id);
	}

	return bResult;
}

void LockWindowStationSession()
{
	typedef BOOLEAN (WINAPI * pWinStationConnect) (HANDLE,ULONG,ULONG,PCWSTR,ULONG);
	typedef BOOL (WINAPI * pLockWorkStation)();

	HMODULE  hlibwinsta = LoadLibrary("winsta.dll"); 
	HMODULE  hlibuser32 = LoadLibrary("user32.dll");

	pWinStationConnect WinStationConnectF=NULL;
	pLockWorkStation LockWorkStationF=NULL;

	if(hlibwinsta)
		WinStationConnectF=(pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW"); 						
	if(hlibuser32)
		LockWorkStationF=(pLockWorkStation)GetProcAddress(hlibuser32, "LockWorkStation"); 						

	if(WinStationConnectF != NULL && WinStationConnectF != NULL) {
		DWORD ID = 0;
		if(g_lpfnWTSGetActiveConsoleSessionId != NULL) 
			ID = g_lpfnWTSGetActiveConsoleSessionId();

		WinStationConnectF(0, 0, ID, L"", 0);
		LockWorkStationF();
	} else {
		Log("Failed to LockWorkStation\n");
	}

	mysleep(3);

	if (hlibwinsta) FreeLibrary(hlibwinsta);
	if (hlibuser32) FreeLibrary(hlibuser32);
}

static BOOL CreateProc(LPCTSTR pszName,LPTSTR pszCmd,PROCESS_INFORMATION &pi)
{
	BOOL bOk = FALSE;
	STARTUPINFO si;

	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(si);

	bOk = ::CreateProcess(pszName,
			pszCmd,							// Program name & path
			NULL, NULL,						// Security attributes
			FALSE,							// Inherit handles?
			CREATE_NO_WINDOW|NORMAL_PRIORITY_CLASS,	// Extra startup flags
			NULL,
			NULL,
			&si,
			&pi);

	return bOk;

#ifdef _CREATEASUSER_TEST
	BOOL         bReturn = FALSE;
	HANDLE       hToken;
	STARTUPINFO  StartUPInfo;
	PVOID        lpEnvironment = NULL;

	static	int	 nFailCount = 0;

	ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
	ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));

	StartUPInfo.wShowWindow = SW_SHOW;
	StartUPInfo.lpDesktop = "Winsta0\\Winlogon";
	//StartUPInfo.lpDesktop = "Winsta0\\Default";
	StartUPInfo.cb = sizeof(STARTUPINFO);
	DWORD dwFlag = 0;

	//
	if(GetSessionUserToken(&hToken)) {
		if(CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE)) {			
			dwFlag = CREATE_UNICODE_ENVIRONMENT |DETACHED_PROCESS;
		} else {
			dwFlag = DETACHED_PROCESS;
			Log("Failed to CreateEnvironmentBlock\n");
		}
		SetLastError(0);
		if(CreateProcessAsUser(hToken,pszName, pszCmd,NULL,NULL,FALSE,
					dwFlag,lpEnvironment,NULL,&StartUPInfo,&pi)) {
			nFailCount = 0;
			bReturn = TRUE;	
		} else {
			int nLastErr =0;
			nLastErr = GetLastError();
			if(nLastErr==233) {
				nFailCount++;
				//if(nFailCount > 3) LockWindowStationSession(); // 				 
			}
			Log("Failed to CreateProcessAsUser : FAIL COUNT(%d), Last Error %d\n", nFailCount, nLastErr);
		}			
		if(lpEnvironment) DestroyEnvironmentBlock(lpEnvironment);

		CloseHandle(hToken);
	} else {
		Log("Failed to GetSessionUserToken\n");
	}

	return bReturn;
#endif
}

static BOOL OpenSvc(char* svc, SC_HANDLE &hScm, SC_HANDLE &hSvc)
{
	char szMsg[512];
	BOOL bOk = FALSE;
	memset(szMsg, 0, sizeof(szMsg));

	hScm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!hScm) {
		mysnprintf(szMsg, sizeof(szMsg), 
			"%s could not be registered as service", svc);
		goto LEnd;
	}

	hSvc = OpenService(hScm, svc, SERVICE_ALL_ACCESS);
	if(!hSvc) {
		mysnprintf(szMsg, sizeof(szMsg), 
			"%s could not be registered as service", svc);
		goto LEnd;
	}
	bOk = TRUE;
LEnd:
	if(bOk) {
	} else {
		fprintf(stderr, "%s\n", szMsg);
	}
	return bOk;
}

// 

//////////////////////////////////////////////////////////////////////
// CEzServiceStatus class
//////////////////////////////////////////////////////////////////////

CEzServiceStatus::CEzServiceStatus()
{
	dwServiceType	= SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
	dwCurrentState	= SERVICE_STOPPED;
	dwControlsAccepted	= SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
	dwWin32ExitCode		= NO_ERROR;
	dwServiceSpecificExitCode	= NO_ERROR;
	dwCheckPoint				= 0;
	dwWaitHint					= 5000;
	m_hStatus					= 0;
}

CEzServiceStatus::~CEzServiceStatus() { }

BOOL CEzServiceStatus::SetState(DWORD dwState,DWORD aCheckPoint,DWORD aWaitHint)
{
	if(dwState != SERVICE_STOPPED) {
		dwWin32ExitCode				= 0;
		dwServiceSpecificExitCode	= 0;
	}

	if(dwState == SERVICE_STOPPED ||
			dwState == SERVICE_PAUSED || dwState == SERVICE_RUNNING) {
		dwCheckPoint	= 0;
		dwWaitHint		= 0;
	} else {
		if(aCheckPoint != (DWORD)-1) dwCheckPoint	= aCheckPoint;
		if(aCheckPoint != (DWORD)-1) dwWaitHint		= aWaitHint;
	}

	dwCurrentState	= dwState;

	return SetServiceStatus(m_hStatus, this);
}

bool CEzServiceStatus::IsAnyRDPSessionActive()
{
	WTS_SESSION_INFO *pSessions = 0;
	DWORD   nSessions = 0;
	DWORD   rdpSessionExists = false;

	if(WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessions, &nSessions)) 
	{
		for(DWORD i = 0; i < nSessions && !rdpSessionExists; ++i)
		{
			if ((_stricmp(pSessions[i].pWinStationName, "Console") != 0) &&
				(pSessions[i].State == WTSActive    || 
				pSessions[i].State  == WTSShadow    ||
				pSessions[i].State  == WTSConnectQuery
				))
			{
				rdpSessionExists = true;
			}
		}

		WTSFreeMemory(pSessions);
	}

	return rdpSessionExists ? true : false;
}

void CEzServiceStatus::DisconnectRemoteSessions()
{	
	// if there's still an active session
	if(IsAnyRDPSessionActive()) return;

	typedef BOOLEAN (WINAPI * pWinStationConnect) (HANDLE,ULONG,ULONG,PCWSTR,ULONG);
	typedef BOOL (WINAPI * pLockWorkStation)();

	HMODULE  hlibwinsta = LoadLibrary("winsta.dll"); 
	HMODULE  hlibuser32 = LoadLibrary("user32.dll");

	pWinStationConnect WinStationConnectF = NULL;
	pLockWorkStation LockWorkStationF = NULL;

	if(hlibwinsta) WinStationConnectF = (pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW"); 	
	if(hlibuser32) LockWorkStationF = (pLockWorkStation)GetProcAddress(hlibuser32, "LockWorkStation"); 
	
	if(WinStationConnectF && WinStationConnectF) {
		DWORD ID = 0;
		if (g_lpfnWTSGetActiveConsoleSessionId!=NULL) 
			ID = g_lpfnWTSGetActiveConsoleSessionId();

		WinStationConnectF(0, 0, ID, L"", 0);
		// sleep to allow the system to finish the connect/disconnect process. and don't locked workstation every time.
		mysleep(3);

		if(!LockWorkStationF())	{
			char msg[1024];
			sprintf(msg, "LockWorkstation failed with error 0x%0X", GetLastError());
			::OutputDebugString(msg);
		}

	}
	mysleep(3);

	if(hlibwinsta) FreeLibrary(hlibwinsta);
	if(hlibuser32) FreeLibrary(hlibuser32);
}

#else

static void signal_handler(int n)
{
	g_pApp->Stop(n);
}

static void signal_sigusr(int n)
{
#ifndef _LINUX
	int pid = getpid();
	if(g_pApp->GetChildPid() == pid) g_pApp->OnSigUsr(n);
#else
	g_pApp->OnSigUsr(n);
#endif
}

#ifndef _OSF1
// using. modified by kskim. 2008.10.23
// not using. modified by kskim. 2008.10.7
// added by hjson. 2008.07.03
static void signal_child(int n)
{
#ifdef _KSKIM // 2008.10.6
	int pid = getpid();
	if(g_pApp->GetChildPid() == pid) // 절대 조건절에 들어올수 없다. 
   	{
		int wstat, cid, count = 0;
		while(count++ < 100) {
			wstat = 0;
			cid = wait3(&wstat,WNOHANG,NULL);
		//	printf("child %d terminated normaly, stat(%d)\n", cid, wstat) ;
			if(cid == 0 || cid == -1) break;
		//	sleep(1);
		}
	} else {
		printf("child terminated normaly, cpid %d, curpid %d\n",
			   	g_pApp->GetChildPid(),pid) ;
	}
#else
	int wstat, cid, count = 0;
	while(count++ < 100) {
		wstat = 0;
		cid = wait3(&wstat,WNOHANG,NULL);
		//printf("child %d terminated normaly, stat(%d)\n", cid,wstat) ;
		if(cid == 0 || cid == -1) break;
		//	sleep(1);
	}
#endif
}
#else
// added by kskim. 2008.07.03
static void signal_child(int signo)
{
	pid_t pid =0;
	int stat, cnt = 0 ;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)  {
	//	printf("child %d terminated normaly, stat(%d)\n", pid,stat) ;
		if(cnt++ > 100) break; //added by hjson. 2008.10.21
	}
	return ;
}
#endif

#endif // #ifdef _WIN32

///////////////////////////////////////////////////////////////////////////////
// class CSvcApp

CSvcApp::CSvcApp()
{
	memset(m_szIniName, 0, sizeof(m_szIniName));
	memset(m_szProgramName, 0, sizeof(m_szProgramName));
	memset(m_szProgramPath, 0, sizeof(m_szProgramPath));
	memset(m_szExecPath, 0, sizeof(m_szExecPath));
	memset(m_szSvcName, 0, sizeof(m_szSvcName));
	memset(m_szWatchDogFile, 0, sizeof(m_szWatchDogFile));
	memset(m_szUpgrade, 0, sizeof(m_szUpgrade));
	memset(m_szUpgradeNew, 0, sizeof(m_szUpgradeNew));
	memset(m_szExecFullName, 0, sizeof(m_szExecFullName));
	memset(m_szEnvPath, 0, sizeof(m_szEnvPath));
	m_bStop = 0;
	m_tStart = 0;
	m_tWatchDog = 0;
	m_bUseWatchDog = 1;
	m_nChildPid = 0;
	g_pApp = this;
#ifdef _WIN32
	m_hStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_nRestartTm = 30;

	// added by jhson 2008.7.18
	// check windows version	
	HMODULE  hmod  = LoadLibrary("kernel32.dll");

	if(hmod) {		
		g_lpfnWTSGetActiveConsoleSessionId = (WTSGETACTIVECONSOLESESSIONID)
											GetProcAddress(hmod,"WTSGetActiveConsoleSessionId"); 
		FreeLibrary(hmod);
	}			

#else
	m_nRestartTm = 30;
#endif
}
//
CSvcApp::~CSvcApp()
{
}


//
void CSvcApp::SetRestartTime(int nSec)
{
	if(nSec >= 1)
		m_nRestartTm = nSec;
}


//
void CSvcApp::Signal()
{
#ifndef _WIN32
	SigAction(SIGTERM,signal_handler);
	SigAction(SIGKILL,signal_handler);
	SigAction(SIGINT,signal_handler);
	SigAction(SIGQUIT,signal_handler);
	SigAction(SIGHUP,signal_handler);
	signal(SIGPIPE,SIG_IGN);
	SigAction(SIGUSR1,signal_sigusr);
	SigAction(SIGUSR2,signal_sigusr);
	// 왓치독을 사용할때 SIGCHLD가 정의되어 있으면 작동하지 않는다.
	// commented by kskim. 2008.10.6
	//SigAction(SIGCHLD,signal_child); // 2008.07.03, hjson@brainz.co.kr

#if defined(POSIX)
    setenv("PATH",_PATH_DEFPATH,1);
#endif
#endif // ifndef _WIN32
}
//
int CSvcApp::IsExistProgram()
{
	FILE *fp;
	int curpid = -1;
	int ppid = -1, cpid =-1, exist = 0;
	char buf[128];

	fp = fopen(m_szPid, "r");
	if(fp) {
		while(fgets(buf, 128, fp)) {
			if(strstr(buf, "PPID")) sscanf(buf, "%*s%d", &ppid);
			if(strstr(buf, "CPID")) sscanf(buf, "%*s%d", &cpid);
		}
		//if(ppid != -1) exist = killchk(m_szProgramName, ppid);
		if(cpid != -1 && !exist) exist = killchk(m_szProgramName, cpid);
		fclose(fp);
	}
	if(!exist) {
#ifdef _WIN32
		curpid = GetCurrentProcessId();
#else
		curpid = getpid();
#endif
		WritePid(ppid, curpid);
	}

	return exist;
}
//
void CSvcApp::KillProgram()
{
#ifdef _WIN32
#else
#endif
}
//
void CSvcApp::WritePid(int ppid, int cpid)
{
	FILE *fp;
	fp = fopen(m_szPid, "w");
	if(fp) {
		fprintf(fp, "PPID  %d\n", ppid);
		fprintf(fp, "CPID  %d\n", cpid);
		fclose(fp);
	}
}
//
void CSvcApp::SetCurDir(char *svcname, char *curdir)
{
	// 2005.08.17. by hjson
	if(curdir == NULL) return;

	// set current directory
#ifndef _WIN32
	strncpy(m_szProgramPath, curdir, sizeof(m_szProgramPath));
#else
	UINT i = 0, j = 0;
	if(strlen(m_szProgramPath) == 0) {
		GetModuleFileName(NULL,m_szExecFullName,sizeof(m_szExecFullName));
		strcpy(m_szProgramPath, m_szExecFullName);
		for(i=0;i<strlen(m_szProgramPath);i++) {
			if(m_szProgramPath[i] == '\\') j = i;
		}
		if(j > 0) m_szProgramPath[j+1] = '\0';
	}
#endif
	strncpy(m_szSvcName, svcname, sizeof(m_szSvcName )-1);
}

//
void CSvcApp::SetProgramPath(char *arg)
{
	// Set current directory
#ifdef _WIN32
	SetCurrentDirectory(m_szProgramPath);
#else
    struct stat sb;

	//  added by kskim. 2008.07.14
	if(strlen(m_szEnvPath)>0) {
		mysnprintf(m_szProgramPath, sizeof(m_szProgramPath),
				"%s/%s", m_szEnvPath, m_szProgramName);
	}

	// first check for current directory
	if (stat(m_szProgramPath, &sb) < 0 && errno == ENOENT) {
		if (mkdir(m_szProgramPath, 0700) == 0) {
			fprintf(stderr, "user %d, %s: created\n", m_nExeUid, m_szProgramPath);
			stat(m_szProgramPath, &sb);
		} else {
			fprintf(stderr, "%s: ", m_szProgramPath);
			perror("mkdir");
			exit(1);
		}
	}
	if (!(sb.st_mode & S_IFDIR)) {
		fprintf(stderr, "'%s' is not a directory, bailing out.\n",
				m_szProgramPath);
		exit(1);
	}
	if (chdir(m_szProgramPath) < 0) {
		fprintf(stderr, "cannot chdir('%s'), bailing out.\n", m_szProgramPath);
		perror(m_szProgramPath);
		exit(1);
	}
#endif

#ifndef _WIN32
	fprintf(stderr, "PROGRAM_PATH=%s\n", m_szEnvPath);
#endif
	fprintf(stderr, "File Path: '%s'\nCurrent path: '%s'\n",
			m_szExecPath, m_szProgramPath);
}

//
void CSvcApp::GetProgramProperty(char *arg)
{
	char *pidx;
	char buf[256];
#ifdef _WIN32
	OSVersionAndPermission();
	GetModuleFileName(NULL,buf,sizeof(buf));
	pidx = strrchr(buf, '\\');
#else
	m_nExeUid = getuid();
	char *pszEnvPath = NULL;
	Signal();
	strcpy(buf, arg);
	pidx = strrchr(buf, '/');
	::GetExecPath(arg, m_szExecPath, sizeof(m_szExecPath));
	pszEnvPath = getenv("PROGRAM_PATH");
	if(pszEnvPath)  strncpy(m_szEnvPath, pszEnvPath, sizeof(m_szEnvPath));
#endif
	if(pidx) {
		strncpy(m_szProgFileName, pidx+1, sizeof(m_szProgFileName));
		strncpy(m_szProgramName, pidx+1, sizeof(m_szProgramName));
		*pidx = '\0';
	} else {
		strncpy(m_szProgFileName, buf, sizeof(m_szProgFileName));
		strncpy(m_szProgramName, buf, sizeof(m_szProgramName));
	}
//#ifdef _WIN32
	pidx = strrchr(m_szProgramName, '.');
	if(pidx) *pidx = '\0';
//#endif
	mysnprintf(m_szIniName,sizeof(m_szIniName), "%s.ini", m_szProgramName);
	mysnprintf(m_szLogName,sizeof(m_szLogName), "%s.log", m_szProgramName);
	mysnprintf(m_szWatchDogFile,sizeof(m_szWatchDogFile),"%s.dog",m_szProgramName);
	mysnprintf(m_szPid,sizeof(m_szPid),"%s.pid",m_szProgramName);
	mysnprintf(m_szUpgrade,sizeof(m_szUpgrade),"%s.upgrade",m_szProgramName);
	mysnprintf(m_szUpgradeNew,sizeof(m_szUpgradeNew),"%s.new",m_szProgramName);

#ifdef _WIN32
	strncpy(m_szExecPath, m_szProgramPath, sizeof(m_szExecPath));
#else
	mysnprintf(m_szExecFullName, sizeof(m_szExecFullName), "%s/%s",
			m_szExecPath, m_szProgFileName);
	struct stat fs;
	if(stat(m_szExecFullName, &fs)<0) {
		fprintf(stderr, "cannot stat('%s'), bailing out.\n", m_szExecFullName);
		Log("SVCAPP: Cannot stat('%s') by user(uid:%d)\n",
				m_szExecFullName, m_nExeUid);
		exit(0);
	}
	m_nOwnerUid = fs.st_uid;
	//m_nGroupUid = fs.st_gid; // added by kskim. 2008.10.20
#endif
}

int CSvcApp::ProcessCommand(int argc, char *argv[])
{
	int i = 0, bRes = 0;

	// get the program properties
	GetProgramProperty(argv[0]);

#ifndef _WIN32
	// Install, Uninstall과정일경우 처리후 바로 리턴한다.
	if(argc > 1) {
		if(!strcmp(argv[1], "-install")) {
			DoInstall(1, argc, argv);
			return 1;
		} 
		if(!strcmp(argv[1], "-uninstall")) {
			DoUninstall(1, argc, argv);
			return 1;
		}
	}
#endif
	SetProgramPath(argv[0]);

	// default ini setting
	if(!ReadIniInt((char*) INI_USEWATCHDOG, m_bUseWatchDog, 1)) {
		WriteIniInt((char*) INI_USEWATCHDOG, m_bUseWatchDog);
		// added by kskim. 2008.10.23
		// *.ini파일의 소유권을 실행자의 것으로 변경한다.
#ifndef _WIN32
		if(m_nExeUid > 0) 
			chown(m_szIniName, getuid(), getgid());
#endif
	}

	// service name setting; 2005.11.11.
	char svcname[64];
	if(ReadIniString((char*) INI_SVCNAME, svcname, sizeof(svcname), m_szSvcName)) {
		strncpy(m_szSvcName, svcname, sizeof(m_szSvcName )-1);
	}

	// pre-process command
	PreProcessCommand(argc, argv);
	if(argc == 1) { // no argument
		bRes = 1;
#ifdef _WIN32
		DoLocService(0, argc, argv); // commented by kskim. 2006.10.23 
		return bRes; // added by kskim. 2006.10.23
#else
		DoService(0, argc, argv);
		return bRes; // added by kskim. 2008.10.6
#endif
	}
	for(i=1; i<argc; i++) {
		if((argv[i][0] == '-') || (argv[i][0] == '/'))  {
			if(DispatchCommand(i, argc, argv)) {
				bRes = 1;
			}
		}
	}
	if(bRes) {
		// post-process command
		PostProcessCommand(argc, argv);
	} else {
		DoHelpMsg(0, argc, argv);
	}

	return bRes;
}

int CSvcApp::DispatchCommand(int idx, int argc, char *argv[])
{
	int nRes = 0;
	char *cmd;
	cmd = argv[idx];

	PCMDMAP		pMap = _GetCmdMap(cmd);
	if(pMap) {
		if(pMap->proc) {
			DispatchCommand(pMap, idx, argc, argv);
			nRes = 1;
		}
	}
	return nRes;
}

void CSvcApp::DispatchCommand(PCMDMAP pMap, int idx, int argc, char *argv[])
{
	CMDPROC			proc;
	proc = pMap->proc;
	(this->*(CMDPROC)proc)(idx, argc, argv);
}
void CSvcApp::WatchDogSet(char *pszTitle)
{
	FILE* fp;
	int okay = 0;
	time_t ttime = time(NULL);

	std::list<_IniItem> qItem;
	_IniItem wdi;
	memset(&wdi,0,sizeof(wdi));

	// read
	fp = fopen(m_szWatchDogFile,"r");
	if(fp) {
		while(fgets(wdi.line,255,fp)) {
			if(strstr(wdi.line,pszTitle)) {
				mysnprintf(wdi.line,255,"%s %ld\n",pszTitle,ttime);
				okay = 1;
			}
			qItem.push_back(wdi);
		}
		fclose(fp);
	}
	if(!okay) {
		mysnprintf(wdi.line,255,"%s %ld\n", pszTitle, ttime);
		qItem.push_back(wdi);
	}

	// write
	fp = fopen(m_szWatchDogFile,"w");
	if(fp) {
		std::list<_IniItem>::iterator it;
		it = qItem.begin();
		while(it != qItem.end()) {
			fprintf(fp,"%s",it->line);
			++it;
		}
		fclose(fp);
	}
}

time_t CSvcApp::WatchDogGet(char *pszTitle)
{
	FILE* fp;
	time_t ttime = 0;

	fp = fopen(m_szWatchDogFile,"r");
	if(fp) {
		char buf[256];
		while(fgets(buf,255,fp)) {
			if(strstr(buf,pszTitle)) {
				sscanf(buf,"%*s%ld",&ttime);
				break;
			}
		}
		fclose(fp);
	}
	
	return ttime;
}

#ifdef _WIN32
void CSvcApp::WatchDog()
{
	int curpid = -1;
	int	nRestart = 0;
	BOOL bPerfInit = TRUE;
	time_t tUpgrade = 0;
	char szCmd[MAX_PATH];
	char szFullPath[MAX_PATH];
	PROCESS_INFORMATION	pi;

	mysnprintf(szFullPath, MAX_PATH-1, "%s%s", 
			m_szProgramPath, m_szProgFileName);
	mysnprintf(szCmd,MAX_PATH-1,"\"%s\" \"-debug\"",szFullPath);

	remove(m_szUpgrade);
	remove(m_szUpgradeNew);
	remove(m_szWatchDogFile);

	if(IsExistProgram()) {
		fprintf(stderr, "%s: already running.\n", m_szProgramName);
		return;
	}
	
	if(CreateProc(szFullPath, szCmd, pi))
	{
		Log("Started %s watchdog on\n", m_szProgramName);
		WritePid(curpid, (int) pi.dwProcessId);
		m_tStart = (UINT) time(NULL);
		while(!m_bStop)
		{
			HANDLE hEvents[2];
			hEvents[0] = pi.hProcess;
			hEvents[1] = m_hStop;

			DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, 3000);

			// GetExitProcessCode();
			if(dwWait == WAIT_OBJECT_0 + 1) { // PP 
				Log("Normally terminated\n");
				TerminateProcess(pi.hProcess, 0);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				break;
			} // CP
			else if(dwWait == WAIT_OBJECT_0 || dwWait == WAIT_ABANDONED) {
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				Log("Second process terminated\n");
				// upgrade agent: 2005.04.18. by kskim
				if(Upgrading()==1) break;

				if(WaitForSingleObject(m_hStop, (DWORD) m_nRestartTm*1000) 
						== WAIT_TIMEOUT) {
					if(CreateProc(szFullPath, szCmd, pi)) { // abnormaly shutdown
						Log("Second process restarted\n");
						WritePid(curpid, (int) pi.dwProcessId);
						nRestart ++;
						m_tStart = (UINT) time(NULL);
					}
				} else 
					break;
			} else if(dwWait == WAIT_TIMEOUT) {
				// check WATCHDOG_RESTART
				time_t tRestart = WatchDogGet(WATCHDOG_RESTART);
				if(tRestart > m_tStart) {
					// restart 확정시
					Log("+++++ WATCHDOG RESTART++++++ (cpid %d)\n", pi.dwProcessId);
					TerminateProcess(pi.hProcess, 0);
				}

				// upgrade agent: 2005.04.18. by kskim
				if(Upgrading(pi.dwProcessId)==1) break;
			} else {
				Log("Unkown reason: watchdog failed (%d)\n", dwWait);
				break;
			}
		}
	}else {
		Log("Failed to start %s\n", m_szProgramName);
	}

#ifdef _KSKIM // modified by kskim. 2009.4.7
	// Kill the process
	TerminateProcess(pi.hProcess, 0); 
#endif
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
#else
void CSvcApp::WatchDog()
{
	char szTemp[128];
	int i, curpid, pid = 0;

    m_tStart = time(NULL);
	curpid = getpid();
	pid = fork();

	// added by kskim 2006.04.26
	sprintf(szTemp, "%s/%s", m_szExecPath, m_szUpgrade);
	remove(szTemp);
	sprintf(szTemp, "%s/%s", m_szExecPath, m_szUpgradeNew);
	remove(szTemp);

#ifndef _LINUX
	if(pid > 0) {	// parent (watchdog) process; 2005.06.10.
		(void)signal(SIGUSR1,SIG_IGN);
		(void)signal(SIGUSR2,SIG_IGN);
	}
#endif

	while(pid>0 && !m_bStop) {
		int   status = 0;
		pid_t wpid =0;

		WritePid(curpid, (int) pid);
		// 절대로 SIGCHLD에 대한 시그널 핸들러가 정의되어 있으면 안됨. (IGN포함)
#ifndef _KSKIM
		while (1) {
			time_t tRestart = WatchDogGet((char*) WATCHDOG_RESTART);
			if(tRestart > m_tStart) { 	// upgrade 확정시
				Log("+++++ WATCHDOG RESTART++++++ (cpid %d)\n", pid);
				killpid(pid, SIGKILL);				// 자식프로세스 죽이기
			}
			mymsleep(100);
			wpid = waitpid(-1, &status, WNOHANG);
			if(wpid!=0) { // 2010.05.27 hjson
				break;
			}
		}
#else
		wpid = wait(&status);
#endif
		// upgrade agent: 2005.05.13. last medified by kskim
		// Upgrading return (-1: upgrade fail 0: no-upgrade, 1: upgrade)
		int upret = Upgrading();

		if(upret != -1) {
			if(wpid == -1) {
				Log("Failed to wait for child\n");
			} else {
				// added by kskim & lordang 2008.10.29
				if(WIFEXITED(status) && !WEXITSTATUS(status)) {
					Log("Child %ld terminated normally.\n", (long)wpid);
				} else if(WIFEXITED(status)) {
					Log("Child %ld terminated with return status %d.\n",
							(long)wpid, WEXITSTATUS(status));
				} else if(WIFSIGNALED(status)) {
					Log("Child %ld terminated due to uncaught signal %d.\n",
							(long)wpid, WTERMSIG(status));
				} else if(WIFSTOPPED(status)) {
					Log("Child %ld stopped due to signal %d.\n", (long)wpid,
							WSTOPSIG(status));
				}
			}
			if(status == 0) {
				Log("Normally terminated\n");
				exit(0);
			}
			Log("Abnormally terminated (status %d)\n",status);
		} else {
			Log("Upgrade failed \n");
		}
		
		// sleep 30 seconds
		for(i=0;i<m_nRestartTm;i++) {
			if(m_bStop) break;
			mysleep(1);
		}

		m_tStart = time(NULL);
		pid = fork();
		if(pid > 0) WritePid(curpid, (int) pid);
	}
	// child process에서 SIGCHLD를 무시하도록함. by kskim. 2008.10.7
	// 관련자료.
	// http://publib.boulder.ibm.com/infocenter/pseries/v5r3/index.jsp
	// When SIGCHLD is set to SIG_IGN, 
	// the signal is ignored and any zombie children of the process 
	// will be cleaned up.
	//signal(SIGCHLD,SIG_IGN);
	SigAction(SIGCHLD,signal_child); // 2008.07.03, hjson@brainz.co.kr
}
#endif

#ifdef _WIN32
DWORD WINAPI CSvcApp::ServiceHandler(DWORD dwControl, DWORD dwEventType, 
									 LPVOID lpEventData, LPVOID lpContext)
{
	CSvcApp* p = g_pApp;
	if(p->m_dwLastControlStatus == dwControl)
		return NO_ERROR;

	switch(dwControl)
	{
	case SERVICE_CONTROL_STOP:
		p->m_dwLastControlStatus = dwControl;
		p->m_status.SetState(SERVICE_STOP_PENDING, 1, 5000);
		// TODO:
		p->Stop();
		SetEvent(p->m_hStop);
		break;
	case SERVICE_CONTROL_PAUSE:
		p->m_dwLastControlStatus = dwControl;
		p->m_status.SetState(SERVICE_PAUSE_PENDING, 1, 5000);
		p->m_status.SetState(SERVICE_PAUSED);
		// TODO:
		p->Stop();
		SetEvent(p->m_hStop);
		break;
	case SERVICE_CONTROL_CONTINUE:
		if(p->m_status.GetState() != SERVICE_RUNNING)
		{
			p->m_dwLastControlStatus = dwControl;
			p->m_status.SetState(SERVICE_CONTINUE_PENDING, 1, 5000);
			// TODO:
			p->m_status.SetState(SERVICE_RUNNING);
		}
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		p->m_dwLastControlStatus = dwControl;
		// TODO:
		p->Stop();
		SetEvent(p->m_hStop);
		break;
	case SERVICE_CONTROL_INTERROGATE:
		//p->m_status.SetState(m_status.GetState());
		break;
	case SERVICE_CONTROL_SESSIONCHANGE:
		{
#if _WIN32_WINNT >= 0x0501
			// This event need to define _WIN32_WINNT=0x0501 in pre-processor option
			if (dwEventType == WTS_REMOTE_DISCONNECT)
			{
				// disconnect rdp, and reconnect to the console
				p->m_status.DisconnectRemoteSessions();
			}
#endif
		}
		break;

	default:
		p->m_dwLastControlStatus = dwControl;
	}

	return NO_ERROR;		 
}

void WINAPI CSvcApp::ServiceMain(DWORD dwArgs, LPTSTR* pszArgv)
{
	CSvcApp* p = g_pApp;
	SERVICE_STATUS_HANDLE hStatus;
	__try
	{
		hStatus = RegisterServiceCtrlHandlerEx(g_pApp->m_szSvcName,
			ServiceHandler, NULL);
		if(hStatus)
		{
			p->m_status.InitStatus(hStatus);
			p->m_status.SetState(SERVICE_START_PENDING, 1, 5000);
			p->m_status.SetState(SERVICE_RUNNING);
			if(p->m_bUseWatchDog) p->WatchDog();
			else {
				g_pApp->m_tStart = (UINT) time(NULL);
				int curpid = GetCurrentProcessId();
				p->WritePid(-1, curpid);
				p->m_nChildPid = curpid;
				Log("Started %s(%d), watchdog off\n", p->m_szProgramName, curpid);
				p->DoStart(0, (int) dwArgs, pszArgv);
				// upgrade agent: 2005.04.18. by kskim
				g_pApp->Upgrading();
			}
			p->m_status.SetState(SERVICE_STOP_PENDING, 1, 5000);
			p->m_status.SetState(SERVICE_STOPPED);
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
	}
	p->m_status.SetState(SERVICE_STOPPED);
}
#endif

int CSvcApp::Upgrading(int pid)
{
	int ret = 0;
	time_t tUpgrade = 0;
	char szCmd[256];
	char szUpgradePath[1024];

	memset(szUpgradePath, 0, 1024);
	memset(szCmd, 0, sizeof(szCmd));

#ifdef _WIN32
	int killret = 1;
	mysnprintf(szUpgradePath,sizeof(szUpgradePath),
		"%s\\%s",m_szExecPath,m_szUpgrade);
	tUpgrade = WatchDogGet(WATCHDOG_UPGRADE);
	if(tUpgrade > m_tStart) { 			// upgrade 확정시
		PROCESS_INFORMATION	piUp;
		mysnprintf(szCmd,sizeof(szCmd),"\"%s\" -upgrade \"%s\"",
				szUpgradePath, m_szUpgradeNew);
		if(pid>0) killret = killpid(pid, 0);
		if(killret > 0) {
			if(CreateProc(szUpgradePath, szCmd, piUp)) {
				int curpid;
				curpid = GetCurrentProcessId();
				Log("Upgrade process started [%s],[%s],pid(%d)\n",
						szUpgradePath, szCmd, piUp.dwProcessId);
				ret = 1;
			}
		}
	}
#else
	mysnprintf(szUpgradePath,sizeof(szUpgradePath),
		"%s/%s",m_szExecPath,m_szUpgrade);
	tUpgrade = WatchDogGet((char*) WATCHDOG_UPGRADE);
	if (tUpgrade > m_tStart) {
		//mysnprintf(szCmd,sizeof(szCmd)," -upgrade \"%s\"", m_szUpgradeNew);
		int exret;
		exret = execl(szUpgradePath, szUpgradePath,
			   	"-upgrade", m_szUpgradeNew, NULL);
		if(exret < 0) ret = -1;
		else ret = 1;
	}
#endif

	return ret;
}

#ifndef _WIN32
static int CopyFile(char* pOri,char* pDst)
{
	char buf[1024];
	int nread, total = 0;
	FILE* fori = fopen(pOri,"rb");
	if(fori) {
		FILE* fdst = fopen(pDst,"wb");
		if(fdst) {
			while((nread=fread(buf,1,1024,fori)) > 0) {
				total += nread;
				fwrite(buf,1,nread,fdst);
			}
			fclose(fdst);
		}
		fclose(fori);
	}
	return total;
}
#endif

int CSvcApp::Upgrade(char *pszFileName)
{
	int ret = 0, cp = 0, bAccess = 0;
	char szUpgradePath[512];

	if((pszFileName) && (strlen(pszFileName) > 0)) {
		strncpy(m_szUpgrade, pszFileName, sizeof(m_szUpgrade));
	}
#ifdef _WIN32
	mysnprintf(szUpgradePath,sizeof(szUpgradePath),
		"%s\\%s",m_szExecPath,m_szUpgrade);
	bAccess = access(szUpgradePath,00);
#else
	mysnprintf(szUpgradePath,sizeof(szUpgradePath),
		"%s/%s",m_szExecPath,m_szUpgrade);
	bAccess = access(szUpgradePath,R_OK);
#endif
	if(bAccess != 0) {
		printf("Upgrade: Cannot access to %s\n",szUpgradePath);
		Log("Upgrade: Cannot access to %s\n",szUpgradePath);
		return ret;
	}
#ifdef _WIN32
	cp = CopyFile(m_szUpgrade, m_szUpgradeNew, FALSE);
	DWORD err = GetLastError();
	if(cp>0) {
#else
	char szUpgradeNewPath[512];
	mysnprintf(szUpgradePath,sizeof(szUpgradePath),
			"%s/%s",m_szExecPath,m_szUpgrade);
	mysnprintf(szUpgradeNewPath,sizeof(szUpgradeNewPath),
			"%s/%s",m_szExecPath,m_szUpgradeNew);
	cp = CopyFile(szUpgradePath, szUpgradeNewPath);
	printf("cp %s %s\n", szUpgradePath,szUpgradeNewPath);
	if(cp>0) {
		struct stat fs;
		if(!stat(m_szExecFullName, &fs)) {
			printf("%s: mod : %d\n", m_szExecFullName, fs.st_mode);
			chmod(szUpgradePath, fs.st_mode);
			printf("%s: chmod : %d\n", szUpgradePath, fs.st_mode);
			chmod(szUpgradeNewPath, fs.st_mode);
			printf("%s: chmod : %d\n", szUpgradeNewPath, fs.st_mode);
		} else {
			printf("Upgrade: failed to stat %s\n", m_szExecFullName);
		}
#endif
		WatchDogSet((char*) WATCHDOG_UPGRADE);
		Stop(); 							// terminate process
		printf("Start upgrade\n");
		Log("Start upgrade\n");
		ret = 1;
	} else if(cp <= 0) {
		remove(m_szUpgradeNew);
		remove(m_szUpgrade);
		ret = 0;
	}

	return ret;
}

#ifdef _WIN32
int CSvcApp::GetOsDesc(char* pOsDesc, int nOsDesc)
{
	char 	szOsv[64];

    OSVERSIONINFOEX osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    _snprintf(szOsv, sizeof szOsv, "Windows");
    if (!GetVersionEx((LPOSVERSIONINFO) &osv)) return -1;

	DWORD major = osv.dwMajorVersion;
	DWORD minor = osv.dwMinorVersion;
	DWORD plfid = osv.dwPlatformId;
#ifdef _HJSON
	DWORD prdtype = osv.wProductType;
	WORD suite = osv.wSuiteMask;
#endif
	switch(major) {
	case 6: // vista or win2008
		if(minor==0) {
			mysnprintf(szOsv, sizeof szOsv, "Windows Vista");			
		}
		break;
	case 4:
		if(minor == 0) { 
			if(plfid == VER_PLATFORM_WIN32_NT) { // win NT4.0
				mysnprintf(szOsv, sizeof szOsv, "Windows NT 4.0");
#ifdef _HJSON
				if(prdtype == VER_NT_WORKSTATION) { // workstation
					mysnprintf(szOsv, 64, "Windows NT 4.0 Workstation");
				} else if(prdtype == VER_NT_DOMAIN_CONTROLLER) { // domain ctrl
					mysnprintf(szOsv, 64, "Windows NT 4.0 Domain Controller");
				} else if(prdtype == VER_NT_SERVER) {
					if(suite & VER_SUITE_ENTERPRISE) { // enterprise
						mysnprintf(szOsv, 64,
								"Windows NT 4.0 Enterprise Edition");
					} else { // server
						mysnprintf(szOsv, 64, "Windows NT 4.0 Server");
					}
				}
#endif
			} else { // win 95
				mysnprintf(szOsv, sizeof szOsv, "Windows 95");
			}
		} else if(minor == 10) { // win 98
			mysnprintf(szOsv, sizeof szOsv, "Windows 98");
		} else if(minor == 90) { // win me
			mysnprintf(szOsv, sizeof szOsv, "Windows ME");
		}
		break;
	case 3: // win NT3.51
		mysnprintf(szOsv, sizeof szOsv, "Windows NT3.51");
		break;
	case 5: // windows 2000, windows XP, windows server 2003 family
		if(minor == 0) { // win 2000
			mysnprintf(szOsv, sizeof szOsv, "Windows 2000");
#ifdef _HJSON
			if(prdtype == VER_NT_WORKSTATION) { // professional
				mysnprintf(szOsv, sizeof szOsv, "Windows 2000 Professional");
			} else if(prdtype == VER_NT_DOMAIN_CONTROLLER) { // domain ctrl
				mysnprintf(szOsv,
						sizeof szOsv, "Windows 2000 Domain Controller");
			} else if(prdtype == VER_NT_SERVER) { // server
				mysnprintf(szOsv, sizeof szOsv, "Windows 2000 Server");
				if(suite & VER_SUITE_ENTERPRISE) { // enterprise
					mysnprintf(szOsv, sizeof szOsv,
							"Windows 2000 Advanced Server");
				} else {
					if(suite & VER_SUITE_DATACENTER) { // datacenter
						strcat(szOsv, " Datacenter Server");
					}
				}
			}
#endif
		} else if(minor == 1) { // win XP
			mysnprintf(szOsv, sizeof szOsv, "Windows XP");
#ifdef _HJSON
			if(prdtype == VER_NT_WORKSTATION) {
				if(suite & VER_SUITE_PERSONAL) { // home edition
					mysnprintf(szOsv, sizeof szOsv, "Windows XP Home Edition");
				} else { // professional
					mysnprintf(szOsv, sizeof szOsv, "Windows XP Professional");
				}
			} else if(prdtype == VER_NT_DOMAIN_CONTROLLER) { // domain ctrl
				mysnprintf(szOsv, sizeof szOsv, "Windows XP Domain Controller");
			} else if(prdtype == VER_NT_SERVER) { // server
				mysnprintf(szOsv, sizeof szOsv, "Windows XP Server");
			}
#endif
		} else if(minor == 2) { // win server 2003 family
			mysnprintf(szOsv, sizeof szOsv, "Windows Server 2003 Family");
#ifdef _HJSON
			if(prdtype == VER_NT_DOMAIN_CONTROLLER) { // domain ctrl
				mysnprintf(szOsv, sizeof szOsv,
						"Windows Server 2003 Domain Controller");
			} else if(prdtype == VER_NT_SERVER) { // server
				mysnprintf(szOsv, sizeof szOsv, "Windows Server 2003");
			}
			if(suite & VER_SUITE_ENTERPRISE) { // enterprise edition
				strcat(szOsv, " Enterprise Edition");
			} else {
				if(suite & VER_SUITE_BLADE) { // web edition
					strcat(szOsv, " Web Edition");
				} 
				if(suite & VER_SUITE_DATACENTER) { // datacenter edition
					strcat(szOsv, " Datacenter Edition");
				} 
			}
#endif
		}
		break;
	}
	if(nOsDesc > 0) {
		if(strlen(osv.szCSDVersion) > 0)
			_snprintf(pOsDesc, nOsDesc, "%s, %s", szOsv, osv.szCSDVersion);
		else
			_snprintf(pOsDesc, nOsDesc, "%s", szOsv);
	}

	return 0;
}
#else
int CSvcApp::GetOsDesc(char *pOsDesc, int nOsDesc)
{
    struct utsname ut;

	if(!pOsDesc) return -1;

	memset(pOsDesc, 0, nOsDesc);
    if (uname(&ut) < 0) return -1;
	
#if		(_AIX)
	mysnprintf(pOsDesc, nOsDesc - 1,
		"%s %s %s", ut.sysname, ut.version, ut.release);
#elif	(_HPUX)
	char	knbits[128];
	memset(knbits, 0, sizeof knbits);
#if		(_HPUX==1020)
	strncpy(knbits, "32", sizeof knbits - 1);
#else
	confstr(_CS_KERNEL_BITS, knbits, sizeof knbits - 1);
#endif	// _HPUX10_20
	mysnprintf(pOsDesc, nOsDesc - 1,
		"%s %s %s", ut.sysname, ut.release, knbits);	
#elif	(_LINUX)
	FILE	*fp;
	mysnprintf(pOsDesc, nOsDesc, "%s %s %s", ut.sysname, ut.release, ut.machine);
	fp = fopen("/etc/debian_version", "r");
	if (fp) {
		char buf[128];
		fgets(buf, sizeof buf - 1, fp);
		mysnprintf(pOsDesc, nOsDesc-1, "GNU Debian Linux %s", buf);
		fclose(fp);
	} else {
		fp = fopen("/etc/redhat-release", "r");
		if (!fp) fp = fopen("/etc/gentoo-release", "r");
		if (fp) {
			char *ptr;
			fgets(pOsDesc, nOsDesc-1, fp);
			ptr = index(pOsDesc, '(');
			if (ptr) *ptr = '\0';
			ptr = index(pOsDesc, '\n');
			if (ptr) *ptr = '\0';
			fclose(fp);
		}
	}
#else
	mysnprintf(pOsDesc,nOsDesc, "%s %s %s", ut.sysname, ut.release, ut.machine);
#endif

	return 0;
}
#endif

void CSvcApp::DoInstall(int idx, int argc, char *argv[])
{
#ifdef _WIN32
	char szMsg[512];
	char szServiceCmd[512];
	SC_HANDLE	hSvc = NULL;
	SC_HANDLE	hScm = NULL;
	BOOL		bOk= FALSE;

	// 2005.11.11. by hjson
	if(argc > idx+1 && argv[idx+1][0]!='-') {
		strncpy(m_szSvcName,argv[idx+1],sizeof(m_szSvcName)-1);
		WriteIniString(INI_SVCNAME,m_szSvcName);
	}

	SERVICE_TABLE_ENTRY mapSvcTable[] = {
		{ m_szSvcName, CSvcApp::ServiceMain },
		{NULL, NULL}
	};

	mysnprintf(szServiceCmd, sizeof(szServiceCmd), "%s\\%s -run",
			m_szProgramPath, m_szProgFileName);

	hScm	= OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!hScm) {
		mysnprintf(szMsg, sizeof(szMsg),
			"%s could not be opended scm", m_szSvcName);
		goto LEnd;
	}

	hSvc = CreateService( hScm, m_szSvcName, m_szSvcName,
					SERVICE_ALL_ACCESS,
					SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
					SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
					szServiceCmd, NULL, NULL, NULL, NULL, NULL);

	if(!hSvc) {
		mysnprintf(szMsg, sizeof(szMsg),
			"%s could not be registered as service",m_szSvcName);
		goto LEnd;
	}
	bOk = TRUE;
	mysnprintf(szMsg,sizeof(szMsg),"%s registered as service",m_szSvcName);
LEnd:
	if(hSvc) CloseServiceHandle(hSvc);
	if(hScm) CloseServiceHandle(hScm);
	if(!bOk) {
	} else {
	}
	fprintf(stderr, "%s\n", szMsg);
#else
	CRcScript *pScript = NULL;
	if(!m_nExeUid) {
		char replyCmd[16] = {"y"};
		char szInstallPath[512];
		char szUser[32] = {"root"}, szCmd[512];
		strncpy(szInstallPath, m_szExecPath, sizeof(szInstallPath));

		// 사용자 존재여부 확인
		// modified by kskim. 2009.01.06
		std::cout << "Execute user <default: root> ?: " << std::ends;
		std::cin.getline(szUser, sizeof(szUser)) ;
		if(strlen(szUser) > 0 && strcmp(szUser, "root")) {
			if(!getpwnam(szUser)) {
#ifdef _KSKIM
				mysnprintf(szCmd, 512, "useradd %s", szUser);
				printf("%s\n",szCmd);
				system(szCmd);
#else
				printf("User <%s> not found. Program will be terminated.\n",
						szUser);
				return;
#endif
			} else {
				// 권한주기
				mysnprintf(szCmd, 512, "chown %s %s/%s", 
						szUser,m_szExecPath, m_szProgFileName);
				printf("%s\n",szCmd);
				system(szCmd);
				if (!access("/bin/sudo", 0)
						|| !access("/usr/sbin/sudo", 0)
						|| !access("/usr/bin/sudo", 0)
						|| !access("/usr/local/bin/sudo", 0))
				{
					// modified by kskim. 2009.01.06
					std::cout <<
						"Do you want to use setting for \"sudo\"(y/n)?"
						"<default: no>";
					std::cin.getline(replyCmd, sizeof(replyCmd));
					if (replyCmd[0] == 'y' || replyCmd[0] == 'Y') {
						memset(szInstallPath, 0, sizeof szInstallPath);
						snprintf(szInstallPath, sizeof szInstallPath - 1, "sudo %s", m_szExecPath);
					}
				}
			}
		} else {
			strcpy(szUser, "root");
		}
		pScript = CRcScript::Create(m_szProgFileName,szInstallPath,szUser);
		fprintf(stderr, "INSTALL: '%s', Path : '%s', User : '%s'\n",
				m_szProgFileName, szInstallPath, szUser);

		if(pScript) {
			pScript->Install();
			//pScript->WriteScript("~/tmp_install.sh");
			fprintf(stderr, "Install completed\n");
		}
		CRcScript::Destroy(pScript);
	} else {
		fprintf(stderr, "Could not install by user(uid:%d) mode. \n"
				"Only root user can install\n", m_nExeUid);
	}
#endif
}

void CSvcApp::DoUninstall(int idx, int argc, char *argv[])
{
#ifdef _WIN32
	char 			szMsg[512];
	SC_HANDLE		hSvc = NULL;
	SC_HANDLE		hScm = NULL;
	BOOL			bOK = FALSE;
	SERVICE_STATUS	ss;

	memset(szMsg, 0, sizeof(szMsg));

	if(!OpenSvc(m_szSvcName, hScm, hSvc)) {
		goto LEnd;
	}

	if(ControlService(hSvc, SERVICE_CONTROL_STOP, &ss)) {
		mysleep(1);
		while(QueryServiceStatus(hSvc, &ss)) {
			if(ss.dwCurrentState == SERVICE_STOP_PENDING)
				mysleep(1);
			else
				break;
		}
		if(ss.dwCurrentState == SERVICE_STOPPED) {
		} else {
			mysnprintf(szMsg, sizeof(szMsg),"%s could not stoped", m_szSvcName);
		}
	}

	if(!DeleteService(hSvc)) {
		mysnprintf(szMsg, sizeof(szMsg), "%s could not removed", m_szSvcName);
		goto LEnd;
	}

	bOK = TRUE;
	mysnprintf(szMsg, sizeof(szMsg), "%s is removed", m_szSvcName);

LEnd:
	if(hSvc) CloseServiceHandle(hSvc);
	if(hScm) CloseServiceHandle(hScm);
	if(!bOK) { } 
	else { }
	fprintf(stderr, "%s\n", szMsg);
#else
	CRcScript *pScript = NULL;
	int uid = (int) getuid();
	if(!uid) {
		pScript = CRcScript::Create(m_szProgFileName,m_szProgramPath);
		if(pScript) {
			pScript->Uninstall();
			//pScript->WriteScript("tmp_install.sh");
		}
		CRcScript::Destroy(pScript);
	} else {
		fprintf(stderr, "Could not uninstall by user(%d) mode. \n"
				"Only root user can uninstall\n", uid);
	}
#endif
}

// start as service (called by service control manager)
void CSvcApp::DoRunService(int idx, int argc, char *argv[])
{
#ifdef _WIN32
	SERVICE_TABLE_ENTRY mapSvcTable[] = {
		{ m_szSvcName, ServiceMain },
		{NULL, NULL}
	};
	StartServiceCtrlDispatcher(mapSvcTable);
#else
	fprintf(stderr, "not suported at unix\n");
#endif
}

// start as service (called by human)
void CSvcApp::DoService(int idx, int argc, char *argv[])
{
#ifdef _WIN32
	char 			szMsg[512];
	SC_HANDLE		hSvc = NULL;
	SC_HANDLE		hScm = NULL;
	BOOL			bOK = FALSE;

	memset(szMsg, 0, sizeof(szMsg));

	if(!OpenSvc(m_szSvcName, hScm, hSvc)) {
		goto LEnd;
	}
	if(!(::StartService(hSvc, 0, NULL))) {
		mysnprintf(szMsg, sizeof(szMsg), 
			"%s service could not be started", m_szSvcName);
		goto LEnd;
	}

	if(m_bUseWatchDog) {
		mysnprintf(szMsg,sizeof(szMsg),"%s started, watchdog on", m_szSvcName);
	}
	else {
		mysnprintf(szMsg,sizeof(szMsg),"%s started, watchdog off", m_szSvcName);
	}

LEnd:
	if(hSvc) CloseServiceHandle(hSvc);
	if(hScm) CloseServiceHandle(hScm);
	fprintf(stderr, "%s\n", szMsg);
#else
	if(IsExistProgram()) {
		fprintf(stderr, "%s: already running.\n", m_szProgramName);
		return;
	}

	int pid = fork();
	switch(pid) {
	case -1:
			fprintf(stderr,"%s: can't start (%d)\n",
							m_szProgramName, (int)getpid());
			exit(0);
			break;
	case 0:
			// child process
			fprintf(stderr,"%s: started (%d)\n",
							m_szProgramName,(int)getpid());
			(void)setsid();
			break;
	default:
			// parent process should just die
			_exit(0);
	}

	if(m_bUseWatchDog) {
		WatchDog();
		m_nChildPid = (int)getpid();	// set child process
		Log("Started %s, watchdog on\n", m_szProgramName);
		if(IsCont()) { // added by kskim. 2009.8.21
			DoStart(idx, argc, argv);
		}
	} else {
		// SIGCHLD 등록. by kskim. 2008.10.23
		//signal(SIGCHLD, SIG_IGN);
		SigAction(SIGCHLD,signal_child); // 2008.07.03, hjson@brainz.co.kr
		//
		m_nChildPid = (int)getpid();	// set child process
		WritePid(-1, m_nChildPid);      // 2005.03.30 by moon
		Log("Started %s, watchdog off\n", m_szProgramName);
		DoStart(idx, argc, argv);
		// upgrade agent: 2005.04.18. by kskim
		Upgrading();
	}
#endif
}

void CSvcApp::DoLocService(int idx, int argc, char *argv[])
{
#ifdef _WIN32
	char szCmd[MAX_PATH];
	PROCESS_INFORMATION	pi;
	_snprintf(szCmd,MAX_PATH-1,"%s -debug",m_szProgFileName);
	if(!CreateProc(m_szProgFileName, szCmd, pi))
		fprintf(stderr, "Cannot start %s.\n", m_szProgramName);
#endif
}

void CSvcApp::OnSigUsr(int nSig)
{
}

void CSvcApp::DoDebug(int idx, int argc, char *argv[])
{
	g_nSvcAppDebugLvl = 1;
	if(IsExistProgram()) {
		fprintf(stderr, "%s: already running.\n", m_szProgramName);
		return;
	}
#ifdef _WIN32
	CThread waitth;
	m_nChildPid = (int)GetCurrentProcessId();
	// {{서비스 모드에서 Event를 받기 위해서 설정을 해야한다.
	// create thread for wait event. added by kskim. 2008.8.25
	char szEvent[64];
	_snprintf(szEvent, 64, "Global\\%s", m_szProgFileName);
	SECURITY_ATTRIBUTES 	sec_attr;
	SECURITY_DESCRIPTOR  	sd;

	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION );
	SetSecurityDescriptorDacl( &sd, TRUE, NULL, FALSE );

	sec_attr.nLength = sizeof(sec_attr);
	sec_attr.bInheritHandle = TRUE;
	sec_attr.lpSecurityDescriptor = &sd;
	m_hEvent = CreateEvent(&sec_attr, FALSE, FALSE, szEvent);
	waitth.Create(WaitEventThread, this);
	waitth.Detach();
	// }}서비스 모드에서 Event를 받기 위해서 설정을 해야한다.
#else
	m_nChildPid = (int)getpid();
#endif
	DoStart(idx, argc, argv);
}

void CSvcApp::DoStop(int idx, int argc, char *argv[])
{
	int res = -1;
#ifdef _WIN32
	char 			szMsg[512];
	SC_HANDLE		hSvc = NULL;
	SC_HANDLE		hScm = NULL;
	BOOL			bOk = FALSE;
	SERVICE_STATUS	ss;

	memset(szMsg, 0, sizeof(szMsg));

	if(!OpenSvc(m_szSvcName, hScm, hSvc)) {
		goto LEnd;
	}

	if(ControlService(hSvc, SERVICE_CONTROL_STOP, &ss)) {
		mysleep(1);
		while(QueryServiceStatus(hSvc, &ss)) {
			if(ss.dwCurrentState == SERVICE_STOP_PENDING) Sleep(1000);
			else break;
		}
		if(ss.dwCurrentState == SERVICE_STOPPED) { }
		else {
			mysnprintf(szMsg,sizeof(szMsg),"%s could not stopped", m_szSvcName);
		}
	} else {
		mysnprintf(szMsg,sizeof(szMsg),"%s could not stopped", m_szSvcName);
	}

	bOk = TRUE;
LEnd:
	if(hSvc) CloseHandle(hSvc);
	if(hScm) CloseHandle(hScm);
	if(!bOk) {
		res = killall(m_szProgramName, 0);
		if(res != -1) fprintf(stderr, "%s is stopped\n", m_szProgFileName);
		else fprintf(stderr, "%s not found\n", m_szProgFileName);
	} else {
		mysnprintf(szMsg, sizeof(szMsg),"%s is stopped", m_szSvcName);
	}
	fprintf(stderr, "%s\n", szMsg);
	Log("Sent signal(Terminate) to %s\n", m_szProgFileName);
#else
	res = killall(m_szProgramName, SIGTERM);
	if(res != -1)
		fprintf(stderr, "%s is stopped\n", m_szProgFileName);
	else
		fprintf(stderr, "%s not found\n", m_szProgFileName);
	Log("Sent signal(SIGTERM) to %s\n", m_szProgFileName);
#endif
}

void CSvcApp::DoKill(int idx, int argc, char *argv[])
{
	int res = -1;
#ifdef _WIN32
	//DoStop(idx, argc, argv);
	res = killall(m_szProgFileName, 0);
#else
	res = killall(m_szProgramName, SIGKILL);
#endif
	if(res != -1)
		fprintf(stderr, "%s is killed\n", m_szProgFileName);
	else
		fprintf(stderr, "%s not found\n", m_szProgFileName);
	Log("Sent signal(SIGKILL) to %s\n", m_szProgFileName);
}

void CSvcApp::DoViewIni(int idx, int argc, char *argv[])
{
	FILE *fp = NULL;
	char buf[1024];
	fp = fopen(m_szIniName, "r");
	if(fp) {
		while(fgets(buf, 1024, fp)) {
			fprintf(stderr, "%s", buf);
		}
		fclose(fp);
	} else {
		fprintf(stderr, "[%s] file not found.\n", m_szIniName);
	}
}

void CSvcApp::DoUpgrade(int idx, int argc, char *argv[])
{
	Log("DoUpgrade Started\n");
	if(argc == 3) {
		strncpy(m_szUpgradeNew, argv[2], sizeof(m_szUpgradeNew)); 
	}

#ifndef	_WIN32
	strncpy(m_szProgFileName, m_szProgramName, sizeof(m_szProgFileName)); 
	chdir(m_szExecPath);

	if (!access(m_szProgFileName, F_OK)) {
		// added by hjson & kskim 2008.10.10
		struct stat fs;
		if(stat(m_szProgFileName,&fs) == 0) {
			chmod(m_szUpgradeNew, fs.st_mode);
			chmod(m_szIniName, fs.st_mode); // added by kskim. 2008.10.20
		}
		//
		int nDelRes = unlink(m_szProgFileName);
		if(nDelRes != 0) nDelRes = remove(m_szProgFileName);
		if(rename(m_szUpgradeNew, m_szProgFileName)) 
			Log("Upgrade failed %s -> %s\n", m_szUpgradeNew, m_szProgFileName);
	} else {
		Log("Upgrade failed, %s is access denied\n", m_szUpgradeNew);
	}

	// execute new agent
#ifdef _KSKIM
	execl(m_szProgFileName, m_szProgFileName, NULL);
#else
	char szFullPath[512];
	mysnprintf(szFullPath, 512, "%s/%s", m_szExecPath, m_szProgFileName);
	execl(szFullPath, szFullPath, NULL);
#endif

#else
	mysnprintf(m_szProgFileName,sizeof(m_szProgFileName),
			"%s.exe", m_szProgramName); 

	// kill previous process : 2004.04.29 by kskim
	killall(m_szProgramName, 0);
	//if (_access(m_szProgFileName, 2) != 0) { // write permission
		mysleep(5); // last modified : 2006.09.20 by kskim
	//}

	int nDelRes = _unlink(m_szProgFileName);
	if(nDelRes != 0) nDelRes = remove(m_szProgFileName); // 재시도

	if(rename(m_szUpgradeNew, m_szProgFileName) != 0)
		Log("Upgrade failed %s -> %s\n", m_szUpgradeNew, m_szProgFileName);

	char szCmd[MAX_PATH];
	PROCESS_INFORMATION	pi;
	_snprintf(szCmd,MAX_PATH-1,"%s -start",m_szProgFileName);
	CreateProc(m_szProgFileName, szCmd, pi);
#endif
}

void CSvcApp::DoShow(int idx, int argc, char *argv[])
{
	int res = -1;
#ifndef _WIN32
	res = killall(m_szProgramName, SIGUSR1);
	if(res == -1)
		fprintf(stderr, "%s not found (%d)\n", m_szProgFileName,res);
#else
	char szEvent[64];
	_snprintf(szEvent, sizeof(szEvent), "Global\\%s", m_szProgFileName);
	HANDLE hEvent;
	hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, szEvent);
	if(hEvent) {
		SetEvent(hEvent);
		fprintf(stderr, "Success to set event!\n");
	} else {
		fprintf(stderr, "%s not found \n", m_szProgFileName);
	}
#endif
}

///////////////////////////////////////////////////////////////
