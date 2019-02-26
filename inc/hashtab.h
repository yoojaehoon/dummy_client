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

//! �����ϰ� ������ Hash table �ڷᱸ�� Ŭ����
class CHashTabInt {
protected:
	UINT			m_uMax;
	UINT			m_uLen;
	_HashTabInt*	m_pDat;

public:
	CHashTabInt(UINT uMax=1024);
	~CHashTabInt();

	//! �ִ밪�� �����Ѵ�.
	UINT	GetMax();
	//! �ִ밪��  �ʱ�ȭ�Ѵ�.
	void	Init(UINT uSize=1024);
	//! ���ؽ��� �����Ѵ�.
	void	GrowRehash(UINT uSize=1024);
	//! ���Ҹ� �߰��Ѵ�.
	UINT	Add(myint64 nId,void* pDat);
	//! �־��� ���Ҹ� �ؽ����� �����ϰ� ���Ҹ� �����Ѵ�.
	void*	Remove(myint64 nId);
	//! �־��� ���Ҹ� �����Ѵ�.
	void*	Find(myint64 nId);
	//! ��� ���Ҹ� �����Ѵ�.
	void	RemoveAll();
};

#endif
