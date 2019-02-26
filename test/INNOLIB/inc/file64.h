/*
   Copyright (c) 2003-2005 BrainzSquare, Inc.
   file64.h - file 64bit i/o operation

   2005.06.10. added ReadLine() by hjson
 */

#ifndef __FILE64_H__
#define __FILE64_H__

#ifndef myint64
#ifdef _WIN32
typedef __int64			myint64;
#else
typedef long long		myint64;
#endif
#endif

#define FILE64_MAX		4096

#define FILE64_SHARE_NONE	0
#define FILE64_SHARE_READ	1
#define FILE64_SHARE_WRITE	2

///////////////////////////////////////////////////////////////////////////////
//
//! 대용량 파일의 읽기 쓰기를 지원하기 위한 클래스
class CFile64 {
#ifdef _WIN32
	HANDLE	m_hFile;
#else
	FILE*	m_fp;
#endif
	char	m_szReadBuf[FILE64_MAX];
	int		m_nReadTot;

public:
	CFile64();
	~CFile64();

	//! 파일 읽기 함수. return 1 if success
	int	Open(char* pPath,
			char* pMode ,
			int nShare=0);	// success(TRUE)
	//! Seek: return the file position
	myint64		Seek(myint64 offset,int origin);
	//! Write: return the number of written bytes
	int			Write(char* val,int size);
	//! Read: return the number of read bytes
	int			Read(char* val,int size);
	//! ReadLine: return the number of read line bytes
	int			ReadLine(char* val,int size);
	//! Close: close the file pointer
	void		Close();
};
///////////////////////////////////////////////////////////////////////////////
//
#endif
