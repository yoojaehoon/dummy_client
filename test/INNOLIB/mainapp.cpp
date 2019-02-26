
#include "stdafx.h"

#ifdef _WIN32
#include <time.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>
#endif

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "dqueue.h"
#include "killall.h"
#include "mainapp.h"
#ifdef _ORACLE
#include "dbmgr.h"
#endif
#include "functime.h"

CMainApp* CMainApp::m_This = NULL;

///////////////////////////////////////////////////////////////////////////////
// CMainApp

BEGIN_CMDMAP(CMainApp)
END_CMDMAP()

CMainApp::CMainApp()
{
	SetCurDir("Zenius sample", "/var/zsample");
	memset(m_szDBTns,0,sizeof(m_szDBTns));
	memset(m_szDBUser,0,sizeof(m_szDBUser));
	memset(m_szDBPass,0,sizeof(m_szDBPass));
	m_port = 6066;
	SetRestartTime(1);
}

CMainApp::~CMainApp()
{
}

CMainApp* CMainApp::InitInstance()
{
	if(m_This == NULL) {
		m_This = new CMainApp;
	}
	return m_This;
}

void CMainApp::ExitInstance()
{
	if(m_This) {
		delete m_This;
	}
}

void CMainApp::Stop(int nSig)
{
	CSvcApp::Stop(nSig);
}

void CMainApp::PreProcessCommand(int argc, char *argv[])
{
	SetRestartTime(10);
}

void CMainApp::DoStop(int idx, int argc, char *argv[])
{
	printf("dostop");
	CSvcApp::DoStop(idx, argc, argv);
}

void CMainApp::DoStart(int idx, int argc, char *argv[])
{
	// support multi-language 2007.07.04. by hjson
	//SetLanguage();

	// get oracle home
	char* pOraHome = getenv("ORACLE_HOME");
	if(pOraHome == NULL) {
		fprintf(stderr,"You must specify ORACLE_HOME at environment\n");
		return;
	}

	// read init profile
	ReadIniString("DBTNS",m_szDBTns,sizeof(m_szDBTns),"zenius");
	ReadIniString("DBUSER",m_szDBUser,sizeof(m_szDBUser),"zenius");
	ReadIniString("DBPASS",m_szDBPass,sizeof(m_szDBPass),"zenius123");

	while(IsCont()) {
		if(!IsCont()) break;

		CFuncTime ft("test");
		for(int i=0; i<5; i++) {
			mysleep(1);
		}
		ft.End();
	}
}

void CMainApp::DoHelpMsg(int idx, int argc, char *argv[])
{
	printf("Usage: %s - Zenius SMS Manager 1.00\n",m_szProgramName);
	printf(" -a <dbtns> <dbuser> <dbpass> : set the DB information\n");
}

void CMainApp::DoVersionMsg(int idx, int argc, char *argv[])
{
	printf("%s ver.0.1.\n", m_szProgramName);
	printf("(c) Brainzsquare,inc. 2005\n");
}

void CMainApp::DoAdd(int idx, int argc, char *argv[])
{
	int num = 4;
	if(argc - idx < num) num = argc - idx;

	if(num >= 4) {
		strncpy(m_szDBTns,argv[idx+1],sizeof(m_szDBTns));
		strncpy(m_szDBUser,argv[idx+2],sizeof(m_szDBUser));
		strncpy(m_szDBPass,argv[idx+3],sizeof(m_szDBPass));
		WriteIniString("DBTNS",m_szDBTns);
		WriteIniString("DBUSER",m_szDBUser);
		WriteIniString("DBPASS",m_szDBPass);
	} else {
		printf("Required 3 arguments : <dbinst> <dbuser> <dbpass>\n");
	}
}

/////////////////////////////////////////////////////////
// Defines the entry point for the console application.

int main(int argc, char	*argv[])
{
	CMainApp *pMain = NULL;
	pMain = CMainApp::InitInstance();

#ifdef _WIN32   // startup socket; needs ws2_32.lib
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD( 1, 1 );
	int err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) return 0;
#endif
	pMain->ProcessCommand(argc, argv);
#ifdef _WIN32   // cleanup socket; needs ws2_32.lib
	WSACleanup();
#endif

	CMainApp::ExitInstance();
	return 0;
}

