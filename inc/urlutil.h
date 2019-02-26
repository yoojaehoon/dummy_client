/*
   Copyright (c) 2005 BrainzSquare, Inc.
   urlutil.h - human readable
   2005.09.23. created by hjson
 */

#include <time.h>

#ifndef __URLUTIL_H__
#define __URLUTIL_H__

// UrlGetToken: return the length of pTok 
int UrlGetToken(char* pData,int& nLen,char* pTok,int nTok,int& bHead);

// UrlGetHostPost: pUrl == http://www.brainz.co.kr/index.jsp
void UrlGetHostPort(char* pUrl,char* pHost,int& nPort,char* pUri);

// UrlEncode
void UrlEncode(char* msg,int msgsize,char* encstr,int encsize);

// UrlDecode
void UrlDecode(char* str,int size);

#endif /* __URLUTIL_H__ */
