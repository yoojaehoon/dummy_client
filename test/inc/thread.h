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

//! Unix, Linux�� Windows�� Thread�� ����ϱ� ���� Ŭ�����μ� Unix, linux�� ��� pthread�� ����Ѵ�.
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
	//! thead stack size�� �������� �Լ�
    size_t	GetStackSize();
	//! thead stack size�� �����ϴ� �Լ�
    static void	SetStackSize(size_t stacksize=CTHREAD_STACK_MIN);
	void 	CreatePThread(THREADPROC pfnProc, LPVOID pParam);
#endif

public:
	CThread();
	virtual ~CThread();

	/** thead �Լ��� �����Ͽ� �����带 �����ϴ� �Լ��μ� ������ �����ϸ� 
	�ٷ� �����尡 ���۵ȴ�. */
	int		Create(THREADPROC pfnc, LPVOID lp);
	/** �����带 �����ϴ� �Լ��μ� ������ �����ϸ� Run�Լ� ������ �����尡 ���۵ȴ�. */
	int		Create();
	/** �������� ���� ���θ� �Ǵ��ϴ� �Լ� */
	int		IsCreated()		{ return m_bCreated;	}
	/** �����尡 �۵� �������� �Ǵ��ϴ� �Լ� */
	int		IsCont()		{ return m_bCont; 		}
	/** �����带 ���� ��Ű�� ���� �Լ� */
	void	Stop()			{ m_bCont = 0; 			}

	// utility functions
	/** �����尡 ����ɶ����� ����ϴ� �Լ�. �����尡 ���� �ɶ����� block�� �ɸ��� �ȴ�. */
	void Join(unsigned int uTimeout=INFINITE);	// milliseconds, only _WIN32
	/** �����带 ���μ������� detach�ϴ� �Լ��μ� pthread_detach�Լ��� ���������� ȣ���Ѵ�. */
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

