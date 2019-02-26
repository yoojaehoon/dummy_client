/*
   Copyright (c) 2000-2005 BrainzSquare, Inc.
   hashtab.h - hash table
 */

#ifndef __HASHTAB_H__
#define __HASHTAB_H__

#ifndef myint64
#ifdef _WIN32
typedef	__int64		myint64;
#else
typedef long long	myint64;
#endif
#endif

struct _HashTabInt {
    myint64	id;
    void*	dat;
    
    _HashTabInt() { id = 0; dat = NULL; }
};

//! 간단하게 구현된 Hash table 자료구조 클래스
class CHashTabInt {
protected:
	UINT			m_uMax;
	UINT			m_uLen;
	_HashTabInt*	m_pDat;

public:
	CHashTabInt(UINT uMax=1024);
	~CHashTabInt();

	//! 최대값을 리턴한다.
	UINT	GetMax();
	//! 최대값을  초기화한다.
	void	Init(UINT uSize=1024);
	//! 재해쉬를 실행한다.
	void	GrowRehash(UINT uSize=1024);
	//! 원소를 추가한다.
	UINT	Add(myint64 nId,void* pDat);
	//! 주어진 원소를 해쉬에서 삭제하고 원소를 리턴한다.
	void*	Remove(myint64 nId);
	//! 주어진 원소를 리턴한다.
	void*	Find(myint64 nId);
	//! 모든 원소를 삭제한다.
	void	RemoveAll();
};

#endif
