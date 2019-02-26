/*
 * aqueue - array queue (thread safe)
 */

#include <stdlib.h>
#include <string.h>

#ifndef _AQUEUE_TEST
#include "mutext.h"
#endif

#ifndef __AQUEUE_TEMPLATE_H__
#define __AQUEUE_TEMPLATE_H__

#ifdef _WIN32
#pragma warning (disable:4786)
#endif

template<class DItem>
class AQueue {
protected:
	char m_szName[32];
	int m_nMax;
	DItem *m_pqList;
	int m_nHead, m_nTail;
	int m_nBegin, m_nEnd, m_nLast;
	int m_nCount, m_nLoss;
#ifndef _AQUEUE_TEST
	CMutext m_mLock;
#endif
    int m_bLock;

public:
	AQueue();
	~AQueue();

	int SetMax(int nMax,char* pName=NULL);	// memory 할당
	int GetMax(); 				// 최대 개수
	bool IsEmpty(); 			// Empty 여부
	void Empty(); 			    // Empty
	DItem* GetCur(); 			// 현재 Tail 값을 return
	DItem* GetNext();           // 현재 Tail 값을 return 하고 advance
	DItem* GetHead();			// 마지막으로 추가된 값 return
	int AddTail(DItem& item); 	// Head 에 추가하고 advance
	int GetCount(); 			// 개수
	int GetLoss();

	void Begin();
	void End();
	int  SetLast();
};

template<class DItem>
AQueue<DItem>::AQueue()
{
	memset(m_szName,0,sizeof(m_szName));
	m_nMax = 0;
	m_pqList = 0;
	m_nHead = m_nTail = 0;
    m_nBegin = m_nEnd = m_nLast = -1;
	m_nCount = 0;
	m_nLoss = 0;
    m_bLock = 0;
}

template<class DItem>
AQueue<DItem>::~AQueue()
{
	if(m_pqList) delete[] m_pqList;
}

template<class DItem>
int AQueue<DItem>::SetMax(int nMax,char* pName)
{
	int nRet = 0;
	if(nMax > 0) {
		m_nMax = nMax;
		m_pqList = new DItem[m_nMax];
		if(m_pqList) nRet = m_nMax;
	}
	if(pName) strncpy(m_szName,pName,sizeof(m_szName)-1);
	return nRet;
}

template<class DItem>
int AQueue<DItem>::GetMax()
{
	return m_nMax;
}

template<class DItem>
bool AQueue<DItem>::IsEmpty()
{
	return (m_nCount>0) ? 0 : 1;
}

template<class DItem>
void AQueue<DItem>::Empty()
{
#ifndef _AQUEUE_TEST
	m_mLock.Lock();
#endif
    m_nHead = m_nTail = 0;
    m_nCount = m_nLoss = 0;
#ifndef _AQUEUE_TEST
	m_mLock.Unlock();
#endif
}

template<class DItem>
DItem* AQueue<DItem>::GetCur()
{
	DItem* pRet = NULL;
#ifndef _AQUEUE_TEST
	m_mLock.Lock();
#endif
	if(m_pqList && m_nHead!=m_nTail) pRet = &m_pqList[m_nTail];
#ifndef _AQUEUE_TEST
	m_mLock.Unlock();
#endif
	return pRet;
}

template<class DItem>
DItem* AQueue<DItem>::GetNext()
{
	DItem* pRet = NULL;
    int nLast = 0;
#ifndef _AQUEUE_TEST
	m_mLock.Lock();
#endif
    nLast = m_nHead;
    if(m_nLast >= 0) nLast = m_nLast;
    //
	if(m_pqList && nLast!=m_nTail) {
		pRet = &m_pqList[m_nTail];
		m_nTail = (m_nTail+1)%m_nMax;
		m_nCount--;
	} else {
        m_nLast = -1;
    }
#ifndef _AQUEUE_TEST
	m_mLock.Unlock();
#endif
	return pRet;
}

template<class DItem>
DItem* AQueue<DItem>::GetHead()
{
	DItem* pRet = NULL;
#ifndef _AQUEUE_TEST
	m_mLock.Lock();
#endif
	if(m_pqList && m_nHead > 0) pRet = &m_pqList[m_nHead-1];
#ifndef _AQUEUE_TEST
	m_mLock.Unlock();
#endif
	return pRet;
}

template<class DItem>
int AQueue<DItem>::AddTail(DItem& item)
{
	if(!m_pqList) return 0;
#ifndef _AQUEUE_TEST
	if(!m_bLock) m_mLock.Lock();
#endif
	m_pqList[m_nHead] = item;
	m_nHead = (m_nHead+1)%m_nMax;
	if(m_nHead == m_nTail) { 	// FULL
		//fprintf(stderr,"AQueue::AddTail Queue is full [%s]\n",m_szName);
		m_nTail = (m_nTail+1)%m_nMax;
		m_nLoss++;
	} else {
		m_nCount++;
	}
#ifndef _AQUEUE_TEST
	if(!m_bLock) m_mLock.Unlock();
#endif
	return m_nCount;
}

template<class DItem>
int AQueue<DItem>::GetCount()
{
	return m_nCount;
}

template<class DItem>
int AQueue<DItem>::GetLoss()
{
	return m_nLoss;
}

template<class DItem>
void AQueue<DItem>::Begin()
{
#ifndef _AQUEUE_TEST
	m_mLock.Lock();
    m_bLock = 1;
#endif
	m_nBegin = m_nHead;
}

template<class DItem>
void AQueue<DItem>::End()
{
	m_nEnd = m_nHead;
#ifndef _AQUEUE_TEST
	m_mLock.Unlock();
    m_bLock = 0;
#endif
}

template<class DItem>
int AQueue<DItem>::SetLast()
{
    m_nLast = -1;
#ifndef _AQUEUE_TEST
	m_mLock.Lock();
#endif
	if(m_nBegin >= 0) {
        m_nTail = m_nBegin;
        m_nBegin = -1;
    }
    //
    if(m_nEnd >= 0) {
        m_nLast = m_nEnd;
        m_nEnd = -1;
    }
#ifndef _AQUEUE_TEST
	m_mLock.Unlock();
#endif
    return m_nLast;
}

int AQueueTest();

#endif
