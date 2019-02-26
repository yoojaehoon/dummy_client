#ifndef __MAINAPP_H__
#define __MAINAPP_H__

#include "condext.h"
#include "tcpssl.h"
#include "svcapp.h"

/** ���α׷��� main ��Ȱ�� �ϴ� Ŭ�����μ� CSvcApp�� ��ӹް� �ִ�. Singleton ������ �Ǿ� �ִ�.
 
 ���α׷� ���۽� �Ʒ��� ���� ���۵Ǹ� \n
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
  
 ���α׷��� DoStart���� ���� �����ϰ� �ȴ�. '-debug' �ɼ��� �ְ� �����ϸ�  \n
 DoStart �Լ��� Debug ���μ� ȣ���ϰ� �ȴ�.  \n
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
	//! ���� �ѹ��� �����Ǹ� ���α׷� ����� ExitInstance�� ȣ��ȴ�.
	static CMainApp* InitInstance();
	//! CMainApp�� ��ü�� ��ȯ�ϴ� �Լ� 
	static CMainApp* Get()	{ return m_This; }
	//! ������ Instance�� �޸𸮿��� �����ϴ� �Լ�
	static void ExitInstance();
	//! ���α׷� ����� ȣ�� �Ǿ�� �ϴ� �Լ�
	void Stop(int nSig=0);

private:

protected:
	CMainApp();
	virtual ~CMainApp();

	/** ���α׷� ���۽� �۵��ϴ� �Լ��μ� main ��Ȱ�� �ϰ� �Ǹ� 
	���� ���α׷� ������ �۵��ϴ� �Լ�*/
	void DoStart(int idx, int argc, char *argv[]);
	/** ���α׷� ���� ���� �ɼ� '-h'�� ���� ó�� �Լ� */
	void DoHelpMsg(int idx, int argc, char *argv[]);
	/** ���α׷� ������ ���� �ɼ� '-v'�� ���� ó�� �Լ� */
	void DoVersionMsg(int idx, int argc, char *argv[]);
	/** ���α׷� �ɼ� '-a'�� ���� ó�� �Լ� */
	void DoAdd(int idx, int argc, char *argv[]);
	/** CSvcApp���� �����ǵ� �Լ��μ� ������� �ʿ��� ���μ����� ����ϴ� �Լ� */
	virtual void DoStop(int idx, int argc, char *argv[]);
	/** CSvcApp���� �����ǵ� �Լ��μ� DoStart�Լ��� ȣ��Ǳ����� ȣ��ȴ�. */
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
