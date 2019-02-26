/*
   Copyright (c) 2002 BrainzSquare, Inc.
   thread.h
   
History:
	2008.12.18 by kskim
		- summary: at hpux, modified CTHREAD_STACK_MIN to 2M
		- modified variable: static size_t m_nStackSize
		- modified function: static void SetStackSize
	2008.12.12 by kskim
		- summary: at aix52, modified CTHREAD_STACK_MIN to 2M
	2008.12.10 by hjson, kskim
		- summary: for _WIN32, modified exception
		- modified variable: added m_nThreadExcept 
	2008.12.3 by hjson, kskim
		- summary: pthread stacksize because hpux1111, aix53
		- modified method: SetStackSize,  GetStackSize, CThread, ~CThread, CreatePThread
		- modified variable: remove m_attr,  rename m_attr_stacksize to m_nStackSize
	2008.11.30 by kskim
		- summary: windows assert window except
		- modified method: Run, ThreadStart
		- modified define: added _EXCEPT_WIN_THREAD 
*/

#ifndef __THREAD_H__
#define __THREAD_H__

#ifndef _WIN32
#include <pthread.h>
#endif

#ifdef _WIN32

//typedef unsigned		THREADTYPE;
typedef unsigned long	THREADTYPE;
typedef THREADTYPE		(*THREADPROC)(LPVOID);
typedef HANDLE			TH_HANDLE;		// thread handle
#else

#ifdef 	_LINUX
#define CTHREAD_STACK_MIN	1048576		// 1M bytes
#elif 	_OSF1
#define CTHREAD_STACK_MIN	524288		// 512k 2008.11.05 hjson
#elif 	_AIX
#define CTHREAD_STACK_MIN	2097152		// 2M 2008.12.12 kskim
#elif 	_HPUX
#define CTHREAD_STACK_MIN	2097152		// 2M 2008.12.18 kskim
#else
#define CTHREAD_STACK_MIN	524288		// 512k 2008.11.05 hjson
#endif // _LINUX


#ifndef INFINITE
#define INFINITE		0xffffffff
#endif // INFINITE

typedef void*			LPVOID;
typedef void*			THREADTYPE;
typedef THREADTYPE		(*THREADPROC)(LPVOID);
typedef pthread_t		TH_HANDLE;		// thread handle
#endif // _WIN32

//! Unix, Linux와 Windows의 Thread를 사용하기 위한 클래스로서 Unix, linux의 경우 pthread를 사용한다.
class CThread
{
public:
	char		m_szName[32];

#ifdef _WIN32
	static int	m_nThreadExcept; // added by kskim. 2008.12.10
#endif

private:
	unsigned long m_uId;
	TH_HANDLE	m_hThread;
	int			m_nAutoDelete;			// for detached thread...
	int			m_bCreated, m_bDetached;
	int			m_bCont;

	//
	LPVOID		m_pParam;
	THREADPROC	m_pfnThreadProc;

#ifndef _WIN32
    static size_t      m_nStackSize;

public:
	//! thead stack size를 가져오는 함수
    size_t	GetStackSize();
	//! thead stack size를 설정하는 함수
    static void	SetStackSize(size_t stacksize=CTHREAD_STACK_MIN);
	void 	CreatePThread(THREADPROC pfnProc, LPVOID pParam);
#endif

public:
	CThread();
	virtual ~CThread();

	/** thead 함수를 지정하여 스래드를 생성하는 함수로서 생성에 성공하면 
	바로 스래드가 시작된다. */
	int		Create(THREADPROC pfnc, LPVOID lp);
	/** 스래드를 생성하는 함수로서 생성에 성공하면 Run함수 내용의 스래드가 시작된다. */
	int		Create();
	/** 스래드의 생성 여부를 판단하는 함수 */
	int		IsCreated()		{ return m_bCreated;	}
	/** 스래드가 작동 중인지를 판단하는 함수 */
	int		IsCont()		{ return m_bCont; 		}
	/** 스래드를 중지 시키기 위한 함수 */
	void	Stop()			{ m_bCont = 0; 			}

	// utility functions
	/** 스래드가 종료될때까지 대기하는 함수. 스래드가 중지 될때까지 block에 걸리게 된다. */
	void Join(unsigned int uTimeout=INFINITE);	// milliseconds, only _WIN32
	/** 스래드를 프로세스에서 detach하는 함수로서 pthread_detach함수를 내부적으로 호출한다. */
	void Detach();

	virtual void Run() {};
	virtual void Exception() {};

private:
#ifdef _WIN32
	static THREADTYPE WINAPI Run(LPVOID lp);
	static THREADTYPE WINAPI ThreadStart(LPVOID lp);
#else
	static THREADTYPE Run(LPVOID lp);
#endif
};

#endif

