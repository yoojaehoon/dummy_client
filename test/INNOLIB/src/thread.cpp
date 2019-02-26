/*
   Copyright (c) 2002 BrainzSquare, Inc.
   thread.cpp - thread

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

#include <stdio.h>

#ifdef	_WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#endif

#include "thread.h"

#ifdef _WIN32
int CThread::m_nThreadExcept = 0;
#else
size_t CThread::m_nStackSize = 0;
#endif

struct ThreadParam {
    THREADPROC  pfnThreadProc;
    LPVOID      pParam;
};

CThread::CThread()
{
	m_pfnThreadProc = NULL;
	m_pParam = NULL;
	//
#ifdef	_WIN32
	m_hThread = NULL;	// thread handle
#endif
	m_nAutoDelete = 0;
	m_uId = 0;
	m_bCreated = 0;
	m_bDetached = 0;
	m_bCont = 1;
	//
#ifndef	_WIN32
#if 	_HPUX != 1020		// HPUX 1020
	if(m_nStackSize == 0) {
		size_t stacksize = GetStackSize();
		if(stacksize && stacksize<CTHREAD_STACK_MIN) 
			SetStackSize(CTHREAD_STACK_MIN);
	}
#endif
#endif
}

CThread::~CThread()
{
#ifdef	_WIN32
	if(m_hThread)	CloseHandle(m_hThread);
#else
	if(m_bCreated && !m_bDetached) Detach();
#endif
}

#ifndef	_WIN32
size_t CThread::GetStackSize()
{
	size_t stacksize = 0;
#if		_HPUX != 1020
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_getstacksize(&attr,&stacksize);
	pthread_attr_destroy(&attr);
#endif
	return stacksize;
}

void CThread::SetStackSize(size_t stacksize)
{
	m_nStackSize = stacksize;
}

void CThread::CreatePThread(THREADPROC pfnProc, LPVOID pParam)
{
#if	(_NCR || _HPUX == 1020)
	if(pthread_create(&m_hThread,pthread_attr_default, 
			pfnProc, pParam) == 0) m_bCreated = 1;
#else
	int ret = 0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if(m_nStackSize>0 && pthread_attr_setstacksize(&attr, m_nStackSize))
		printf("CThread: failed to set statck size\n");

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL);
	ret = pthread_create(&m_hThread,&attr, pfnProc, pParam);
	pthread_attr_destroy(&attr);
	if(ret == 0) m_bCreated = 1;
	else printf("CThread: pthread_create failed (%d)\n", ret);
#endif
}

#endif

int CThread::Create(THREADPROC pfnProc,LPVOID pParam)
{
	m_pfnThreadProc = pfnProc;
	m_pParam = pParam;
	m_bCont = 1;
#ifdef	_WIN32
	ThreadParam* pThreadParam = new ThreadParam;
	pThreadParam->pfnThreadProc = m_pfnThreadProc;
	pThreadParam->pParam = m_pParam;
	m_hThread = (HANDLE)CreateThread(NULL,0,ThreadStart,
		pThreadParam,0,&m_uId);
	if(m_hThread)
		m_bCreated = 1;
#else
	CreatePThread(m_pfnThreadProc, m_pParam);
#endif
	if(m_bCreated) {
#if 0
#ifdef	_WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
#endif
	}
	return m_bCreated;
}

int CThread::Create()
{
	if(m_bCreated) return 0;

	m_bCont = 1;
#ifdef	_WIN32
	m_hThread = (HANDLE)CreateThread(NULL,0,Run,this,0,&m_uId);
	
	if(m_hThread) {
		m_bCreated = 1;
	}
#else // added by kskim. 2008.9.23
	CreatePThread(Run, this);
#endif

	if(m_bCreated) {
#if 0
#ifdef	_WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
#endif
	}

	return m_bCreated;
}

void CThread::Join(unsigned int uTimeout)
{
#ifdef	_WIN32
	if(m_hThread == NULL) return;
	if(WAIT_OBJECT_0 != WaitForSingleObject(m_hThread,uTimeout)) {
		::TerminateThread(m_hThread, 0);
	}
	CloseHandle(m_hThread);
	m_hThread = NULL;
#else
	pthread_join(m_hThread,NULL);
#endif
	m_bCreated = 0;
}

void CThread::Detach()
{
	m_bDetached = 1;
#ifdef	_WIN32
	if(m_hThread) {
		CloseHandle(m_hThread);
	    m_hThread = NULL;
	}
#elif	_NCR
	pthread_detach(&m_hThread);
#else
	pthread_detach(m_hThread);
#endif
	m_bCreated = 0;
}

#ifdef	_WIN32
THREADTYPE WINAPI CThread::Run(LPVOID pParam)
{
	CThread* _this = (CThread*) pParam;
	if(m_nThreadExcept) {
		__try
		{
			_this->Run();
		}__except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? 
				EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
			_this->Exception();
			ExitProcess(0);
		}
	} else {
		_this->Run();
	}

	_this->m_bCreated = 0;		// 2005.04.06. by hjson
	_this->m_bCont = 0; 		// 2006.02.16. by kskim
	ExitThread(0);
	return 0;
}
#else
THREADTYPE CThread::Run(LPVOID pParam)
{
	CThread* _this = (CThread*) pParam;
	_this->Run();
	_this->m_bCreated = 0;		// 2005.04.06. by hjson
	_this->m_bCont = 0; 		// 2006.02.16. by kskim
	return 0;
}
#endif

#ifdef	_WIN32
THREADTYPE WINAPI CThread::ThreadStart(LPVOID pParam)
{
	ThreadParam* pThreadParam = (ThreadParam*)pParam;
	if(m_nThreadExcept) {
		__try
		{
			pThreadParam->pfnThreadProc(pThreadParam->pParam);
		}__except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? 
			EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
			ExitProcess(0);
		}
	} else {
		pThreadParam->pfnThreadProc(pThreadParam->pParam);
	}
	delete pThreadParam;
	ExitThread(0);
	return 0;
}
#endif
