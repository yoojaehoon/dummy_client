/*
   Copyright (c) 2002 BrainzSquare, Inc.
   ref.h - reference counter
 */

#include <stdio.h>
//#include "utils.h"

#ifndef __REF_H__
#define __REF_H__

//! 메모리 관리를 위한 Reference counter class
class CRef
{
	unsigned int m_nRef;
	unsigned int m_nState;

public:
	CRef() { m_nRef = 0; m_nState = 1; }
	virtual ~CRef() { }

	void Ref() { m_nRef++; }
	void Unref() { m_nRef--; if(m_nRef == 0) delete this; }
	unsigned int GetRef() { return m_nRef; }

	virtual void SetCont() { m_nState = 1; }
	virtual void SetStop() { m_nState = 0; }
	unsigned int IsCont() { return m_nState; }
};

#endif
