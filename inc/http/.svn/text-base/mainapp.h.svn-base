#ifndef __MAINAPP_H__
#define __MAINAPP_H__

#include "condext.h"
#include "tcpssl.h"
#include "svcapp.h"

/** 프로그램의 main 역활을 하는 클래스로서 CSvcApp를 상속받고 있다. Singleton 구조로 되어 있다.
 
 프로그램 시작시 아래와 같이 시작되며 \n
 int main(int argc, char *argv[]) \n
 { \n
	  CMainApp *pMain = NULL; \n
	  pMain = CMainApp::InitInstance(); \n

   \#ifdef _WIN32   // startup socket; needs ws2_32.lib  \n
	  WSADATA wsaData;  \n
   WORD wVersionRequested = MAKEWORD( 1, 1 );  \n
   int err = WSAStartup( wVersionRequested, &wsaData );  \n
   if ( err != 0 ) return 0;  \n
   \#endif  \n
 	pMain->ProcessCommand(argc, argv);  \n
   \#ifdef _WIN32   // cleanup socket; needs ws2_32.lib  \n
   WSACleanup();  \n
   \#endif  \n
   CMainApp::ExitInstance();  \n
   return 0;  \n
  }  \n
  
 프로그램은 DoStart에서 부터 시작하게 된다. '-debug' 옵션을 주고 시작하면  \n
 DoStart 함수가 Debug 모드로서 호출하게 된다.  \n
 void CMainApp::DoStart(int idx, int argc, char *argv[]) \n
 { \n
  // support multi-language 2007.07.04. by hjson \n
 	ReadIniString("DBPASS",m_szDBPass,sizeof(m_szDBPass),"zenius123"); \n
 
 	// open database connection pool \n
 	CDBMgr::OpenPool(m_szDBTns,m_szDBUser,m_szDBPass); \n
 	//CDBMgr::SetGlobalAutoCommit(1); \n
 
  // DO IT  \n \n
 	// close database connection pool \n
 	CDBMgr::ClosePool(); \n
 } 
 */
class CMainApp : public CSvcApp
{
	DECLARE_CMDMAP();

// functions
public:
	//! 최초 한번만 생성되며 프로그램 종료시 ExitInstance가 호출된다.
	static CMainApp* InitInstance();
	//! CMainApp의 객체를 반환하는 함수 
	static CMainApp* Get()	{ return m_This; }
	//! 생성된 Instance를 메모리에서 해제하는 함수
	static void ExitInstance();
	//! 프로그램 종료시 호출 되어야 하는 함수
	void Stop(int nSig=0);

private:

protected:
	CMainApp();
	virtual ~CMainApp();

	/** 프로그램 시작시 작동하는 함수로서 main 역활을 하게 되며 
	실제 프로그램 구동시 작동하는 함수*/
	void DoStart(int idx, int argc, char *argv[]);
	/** 프로그램 설명에 대한 옵션 '-h'에 대한 처리 함수 */
	void DoHelpMsg(int idx, int argc, char *argv[]);
	/** 프로그램 버전에 대한 옵션 '-v'에 대한 처리 함수 */
	void DoVersionMsg(int idx, int argc, char *argv[]);
	/** 프로그램 옵션 '-a'에 대한 처리 함수 */
	void DoAdd(int idx, int argc, char *argv[]);
	/** CSvcApp에서 재정의된 함수로서 종료시의 필요한 프로세스를 담당하는 함수 */
	virtual void DoStop(int idx, int argc, char *argv[]);
	/** CSvcApp에서 재정의된 함수로서 DoStart함수가 호출되기전에 호출된다. */
	virtual void PreProcessCommand(int argc, char *argv[]);

// variables
public:
	char    m_szDBTns[64];
	char    m_szDBUser[64];
	char    m_szDBPass[64];
	int		m_port;

private:

protected:
	static CMainApp *m_This;
};

#endif
