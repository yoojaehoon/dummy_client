/*
 * debug.h - debugging information
 * Copyright (c) 2005 Brainzsquare, Inc.
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

// added by azrael for debug
struct _TRACEINFO
{
    static char*    m_pFileName;
    static int      m_nLineNum;
    static void     _Trace(const char* format, ...);
};

extern void _TRACE(const char* format, ...);
inline void _NOTRACE(const char* format, ...) {}

//#define NOTRACE     1 ? (void)0 : _NOTRACE
#define ONTRACE \
    _TRACEINFO::m_pFileName = __FILE__, \
    _TRACEINFO::m_nLineNum = __LINE__, \
    _TRACEINFO::_Trace

//#ifdef _DEBUG
//! ���α׷��� �����ڵ�� ���μ��� �Բ� �޼����� ȭ�鿡 ����Ѵ�.
//! ����׸��� ����� ���¿����� ��ȿ�ϴ�.
//! ���� : TRACE("hello trace %s line\n", var);
#define TRACE       ONTRACE
//#else
//#define TRACE       NOTRACE
//#endif // _DEBUG

#endif //__DEBUG_H__
