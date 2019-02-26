
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

//#include "dqueue.h"
#include "killall.h"
#include "mainapp.h"
#ifdef _ORACLE
#include "dbmgr.h"
#endif
//#include "functime.h"

#include "src/header.h"

CMainApp* CMainApp::m_This = NULL;

///////////////////////////////////////////////////////////////////////////////
// CMainApp

BEGIN_CMDMAP(CMainApp)
END_CMDMAP()

CMainApp::CMainApp()
{
	SetCurDir("LOG", "/inno_service/ClouditMSService/conf");
//	memset(m_szDBTns,0,sizeof(m_szDBTns));
//	memset(m_szDBUser,0,sizeof(m_szDBUser));
//	memset(m_szDBPass,0,sizeof(m_szDBPass));
//	m_port = 6066;
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
/*	char* pOraHome = getenv("ORACLE_HOME");
	if(pOraHome == NULL) {
		fprintf(stderr,"You must specify ORACLE_HOME at environment\n");
		return;
	}

	// read init profile
	ReadIniString("DBTNS",m_szDBTns,sizeof(m_szDBTns),"zenius");
	ReadIniString("DBUSER",m_szDBUser,sizeof(m_szDBUser),"zenius");
	ReadIniString("DBPASS",m_szDBPass,sizeof(m_szDBPass),"zenius123");
*/
/*	while(IsCont()) {
		if(!IsCont()) break;

		CFuncTime ft("test");
		for(int i=0; i<5; i++) {
			mysleep(1);
		}
		ft.End();
	}
*/
}

void CMainApp::DoHelpMsg(int idx, int argc, char *argv[])
{
	printf("Usage: %s - ClouditMS Manager 1.00\n",m_szProgramName);
	printf(" -a <dbtns> <dbuser> <dbpass> : set the DB information\n");
}

void CMainApp::DoVersionMsg(int idx, int argc, char *argv[])
{
	printf("%s ver.0.1.\n", m_szProgramName);
	printf("(c) Innogrid,inc. 2011\n");
}

void CMainApp::DoAdd(int idx, int argc, char *argv[])
{
/*	int num = 4;
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
*/
}

/////////////////////////////////////////////////////////
// Defines the entry point for the console application.
#include <mysql.h>
#include "src/Database.h"
#include "src/CloudMSApi.h"
#include "src/ApnsApple.h"
#include "src/CommonThread.h"

// Config 
int  HTTP_PORT;
char LOG_PATH[256];
int  DATABASE_POOL_SIZE;
char DATABASE_SERVER[256];
char DATABASE_USER[128];
char DATABASE_PASSWORD[128];
char DATABASE_DATABASE[128];
int  APNS_POOL_SIZE;
int  FD_LIMIT;
//int  MESSAGE_KEEP_DATE;


#include "http_service.cpp"

//#define HTTP_PORT 80
//#define FDLIMIT	  10000

int main(int argc, char	*argv[])
{
	// 기본 설정값 conf/ClouditMSService.conf 파일에서 읽어옴.
	FILE *file;

	file = (FILE *)fopen("/inno_service/ClouditMSService/conf/ClouditMSService.conf", "r");

	if( file == NULL )
	{
		fprintf(stderr, "conf/ClouditMSService.conf Not Found\nStart FAILED\n");
		exit(EXIT_FAILURE);
	}
	
	char buffer[500];
	int confcount = 0;

	while(1)
	{
		memset(buffer, 0, sizeof(buffer));
		fscanf(file, "%[^\n]\n", buffer);

		if( strlen(buffer) == 0 )
			break;

		switch( confcount )
		{
			case 0:
				sscanf(buffer, "%d", &HTTP_PORT); break;
			case 1:
				sscanf(buffer, "%s", LOG_PATH);	break;
			case 2:
				sscanf(buffer, "%d", &DATABASE_POOL_SIZE); break;
			case 3:
				sscanf(buffer, "%s", DATABASE_SERVER); break;
			case 4:
				sscanf(buffer, "%s", DATABASE_USER); break;
			case 5:
				sscanf(buffer, "%s", DATABASE_PASSWORD); break;
			case 6:
				sscanf(buffer, "%s", DATABASE_DATABASE); break;
			case 7:
				sscanf(buffer, "%d", &APNS_POOL_SIZE); break;
			case 8:
				sscanf(buffer, "%d", &FD_LIMIT); break;
			case 9:
				sscanf(buffer, "%d", &CCommonThread::message_keep_date); break;
		}
		confcount++;
	}
	fclose(file);

	printf("HTTP_PORT - %d\n", HTTP_PORT);
	printf("LOG_PATH - %s\n", LOG_PATH);
	printf("DATABASE_POOL_SIZE - %d\n", DATABASE_POOL_SIZE);
	printf("DATABASE_SERVER - %s\n", DATABASE_SERVER);
	printf("DATABASE_USER - %s\n", DATABASE_USER);
	printf("DATABASE_PASSWORD - %s\n", DATABASE_PASSWORD);
	printf("DATABASE_DATABASE - %s\n", DATABASE_DATABASE);
	printf("APNS_POOL_SIZE - %d\n", APNS_POOL_SIZE);
	printf("FD_LIMIT - %d\n", FD_LIMIT);
	printf("MESSAGE_KEEP_DATE - %d\n", CCommonThread::message_keep_date);

	///////////////////////////////////////////////////////
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
	
	// Log 초기화
	CMiscUtil::InitLog(LOG_PATH);
	
	// fd open 개수 설정 //////////////////////////////////
	int result;
	struct rlimit new_rl;
	new_rl.rlim_cur = FD_LIMIT;
	new_rl.rlim_max = FD_LIMIT;

	if( setrlimit(RLIMIT_NOFILE, &new_rl) < 0 )
	{
		fprintf(stderr, "SET FD LIMIT FAILED\n");
		exit(EXIT_FAILURE);
	}

	CMiscUtil::InitLog(LOG_PATH);

	// DB
	my_init();

	for( int i=0; i<100; i++ )
	{
		result = CDatabase::Init(DATABASE_SERVER, DATABASE_USER, DATABASE_PASSWORD, DATABASE_DATABASE, DATABASE_POOL_SIZE);

		if( !result )
		{
			fprintf(stderr, "DATABASE INITIALIZE FAILD\n");

			if( i == 99 )
			{
				fprintf(stderr, "DATABASE CONNECT 100 OVER...FAILED\n");
				exit(EXIT_FAILURE);
			}
			else
			{
				CDatabase::Destroy();
				sleep(30);
			}
		}
		else
			break;
	}
	
	// 알람 메시지 Thread ////////////////////////////////
	pthread_t c_thread;
	pthread_attr_t thread_attr;
	
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	
	// APNS 처리 Thread Pool
	result = pthread_mutex_init(&CCommonThread::g_ApnsPoolLock, NULL);
	if( result )
	{
		fprintf(stderr, "APNS POOL MUTEX INITIALIZE FAILED\n");
		exit(EXIT_FAILURE);
	}

	result = sem_init(&CCommonThread::g_ApnsPoolSemaphore, 0, 0);

	if( result )
	{
		fprintf(stderr, "APNS POOL SEMAPHORE INITIALIZE FAILED\n");
		exit(EXIT_FAILURE);
	}
	for( int i=0; i < APNS_POOL_SIZE; i++ )
	{
		int *thread = new int;
		*thread = i+1;

		result = pthread_create(&c_thread, &thread_attr, apns_pool_thread, (void *)thread);

		if( result )
		{
			fprintf(stderr, "APNS POOL THREAD CREATE FAILE\n");
			exit(EXIT_FAILURE);
		}
	}

	// APNS 보내지 못한 메시지 처리 Thread
	result = pthread_create(&c_thread, &thread_attr, apns_thread, NULL);

	if( result != 0 )
	{
		fprintf(stderr, "APNS THREAD CREATE FAILED\n");
		exit(EXIT_FAILURE);
	}

	// APP에서 다운받은 메시지 지우기
	result = pthread_create(&c_thread, &thread_attr, message_clean_thread, NULL);

	if( result != 0 )
	{
		fprintf(stderr, "MESSAGE CLEAN THREAD CREATE FAILED\n");
		exit(EXIT_FAILURE);
	}

	// Rest Service 실행 /////////////////////////////////
	MHD_Daemon* daemon = MHD_start_daemon(
					MHD_USE_THREAD_PER_CONNECTION, HTTP_PORT,
					callback_client_connect, NULL,
					&callback_request_to_connect, NULL,
					MHD_OPTION_NOTIFY_COMPLETED, callback_request_completed,
					NULL, MHD_OPTION_END);
	if( daemon == NULL )
	{
		fprintf(stderr, "MHD_start_daemon CREATE FAILED\n");
		exit(EXIT_FAILURE);
	}

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	// SIGPIPE 무시 설정
	signal(SIGPIPE, SIG_IGN);

	while(1)
	{
		sleep(1000);
	}

	MHD_stop_daemon(daemon);
	CMainApp::ExitInstance();
	
	exit(EXIT_SUCCESS);		
}

