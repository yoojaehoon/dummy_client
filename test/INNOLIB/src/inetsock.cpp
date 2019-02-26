/*
 * InetSocket
 * Copyright (c) 2011 Brainzsquare, Inc.
 *
 * 2010.12.01. created by kskim
 */

#include "stdafx.h"

#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _AIX
#include <strings.h>
#endif

#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include "inetsock.h"
#include "debug.h"

#ifndef AF_INET6
#define AF_INET6 	10 // linux /usr/include/bits/socket.h 참조
#endif

float htonf(float val)
{
	float fv;
	int nv;

	if(1 == htonl(1)) {
		return val;
	}

	memcpy(&nv,&val,4);
	nv = htonl(nv);
	memcpy(&fv,&nv,4);

	return fv;
}

float ntohf(float val)
{
	float fv;
	int nv;

	if(1 == ntohl(1)) {
		return val;
	}

	memcpy(&nv,&val,4);
	nv = ntohl(nv);
	memcpy(&fv,&nv,4);

	return fv;
}

double htond(double val)
{
	char ps[sizedouble];
	int high, low;

	if(1 == htonl(1)) {
		return val;
	}

	// copy value to string
	memcpy(ps,&val,sizedouble);
	memcpy(&low,ps,sizeint);
	memcpy(&high,&ps[sizeint],sizeint);

	high = htonl(high);
	low = htonl(low);

	memcpy(ps,&high,sizeint);
	memcpy(&ps[sizeint],&low,sizeint);

	// copy string to value
	memcpy(&val,ps,sizedouble);

	return val;
}

double ntohd(double val)
{
	char ps[sizedouble];
	int high, low;
	
	if(1 == ntohl(1)) {
		return val;
	}

	// copy value to string
	memcpy(ps,&val,sizedouble);

	memcpy(&high,ps,sizeint);
	memcpy(&low,&ps[sizeint],sizeint);	

	high = ntohl(high);
	low = ntohl(low);

	memcpy(ps,&low,sizeint);
	memcpy(&ps[sizeint],&high,sizeint);

	// copy string to value
	memcpy(&val,ps,sizedouble);

	return val;
}

myint64 htonint64(myint64 val)
{
	char ps[sizeint64];
	int high, low;

	if(1 == htonl(1)) {
		return val;
	}

	// copy value to string
	memcpy(ps,&val,sizeint64);
	memcpy(&low,ps,sizeint);
	memcpy(&high,&ps[sizeint],sizeint);

	high = htonl(high);
	low = htonl(low);

	memcpy(ps,&high,sizeint);
	memcpy(&ps[sizeint],&low,sizeint);

	// copy string to value
	memcpy(&val,ps,sizeint64);

	return val;
}

myint64 ntohint64(myint64 val)
{
	char ps[sizeint64];
	int high, low;

	if(1 == ntohl(1)) {
		return val;
	}

	// copy value to string
	memcpy(ps,&val,sizeint64);

	memcpy(&high,ps,sizeint);
	memcpy(&low,&ps[sizeint],sizeint);	

	high = ntohl(high);
	low = ntohl(low);

	memcpy(ps,&low,sizeint);
	memcpy(&ps[sizeint],&high,sizeint);

	// copy string to value
	memcpy(&val,ps,sizeint64);

	return val;
}

umyint64 uiuitouint64(unsigned int low,unsigned int high)
{
	umyint64 val = 0;
	val = high; val = val << 32; val |= low;

	return val;
}

myint64 uiuitoint64(unsigned int low,unsigned int high)
{
	myint64 val = 0;
	val = high; val = val << 32; val |= low;

	return val;
}

void int64touiui(myint64 val,unsigned int& low,unsigned int& high)
{
	low = (unsigned int)(val & 0xffffffff);
	high = (unsigned int)(val >> 32);
}

const char *myinet_ntop(int af, const void *src, char *dst, mysocklen_t cnt)
{
#ifdef _WIN32
	// myproc inet_ntop의 함수원형과 같은 타입이어야 한다.
	typedef  const char* (*myproc)(int , const void* ,char*, size_t);

	// 
	const char *res = NULL;
    HINSTANCE hinstLib;
	myproc proc;
    BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
 
    // Get a handle to the DLL module. 
    hinstLib = LoadLibrary("Ws2_32.dll");
	if (hinstLib != NULL) 
	{ 
		proc = (myproc) GetProcAddress(hinstLib, "inet_ntop"); 
		// If the function address is valid, call the function.
		if(NULL != proc) {
			fRunTimeLinkSuccess = TRUE;
		}
	}
	if(fRunTimeLinkSuccess) {
		// inet_ntop 가 정의되어 있다면inet_ntop 실행
		// Free the DLL module.
		res = (proc) (af, src, dst, cnt);
 		fFreeResult = FreeLibrary(hinstLib);
		return res;
	} else {
		// inet_ntop 가 정의되어 있지 않다면
		if (af == AF_INET)
		{
			struct sockaddr_in in;
			memset(&in, 0, sizeof(in));
			in.sin_family = AF_INET;
			memcpy(&in.sin_addr, src, sizeof(struct in_addr));
			getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst,
					cnt, NULL, 0, NI_NUMERICHOST);
		}
		else if (af == AF_INET6)
		{
			struct sockaddr_in6 in;
			memset(&in, 0, sizeof(in));
			in.sin6_family = AF_INET6;
			memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
			getnameinfo((struct sockaddr *)&in, sizeof(struct
						sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
		}
	}
	if(hinstLib) {
		// Free the DLL module.
 		fFreeResult = FreeLibrary(hinstLib);
	}
	return dst;
#else
	return inet_ntop(af, src, dst, cnt);
#endif
}

int INET_PTON(int af, const char *src, void *dst)
{
#ifdef _WIN32
	// myproc inet_pton의 함수원형과 같은 타입이어야 한다.
	typedef  int (*myproc)(int, const char*, void*);

	// 
	int nres = 0;
    HINSTANCE hinstLib;
	myproc proc;
    BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
 
    // Get a handle to the DLL module. 
    hinstLib = LoadLibrary("Ws2_32.dll");
	if (hinstLib != NULL) 
	{ 
		proc = (myproc) GetProcAddress(hinstLib, "inet_pton"); 
		// If the function address is valid, call the function.
		if(NULL != proc) {
			fRunTimeLinkSuccess = TRUE;
		}
	}

	if(fRunTimeLinkSuccess) {
		nres = (proc)(af, src, dst);
 		fFreeResult = FreeLibrary(hinstLib);
		return nres;
	}
	//
	if(hinstLib) {
		// Free the DLL module.
 		fFreeResult = FreeLibrary(hinstLib);
	}

	struct addrinfo hints, *res, *ressave;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = af;

	if (getaddrinfo(src, NULL, &hints, &ressave) != 0)
	{
		printf("Couldn't resolve host %s\n", src);
		return -1;
	}

	for(res=ressave; res!=NULL; res = res->ai_next ) {
		memcpy(dst, res->ai_addr, res->ai_addrlen);
	}
	freeaddrinfo(ressave);
	return 0;
#else
	return inet_pton(af, src, dst);
#endif
}

int InetSocket::SetNonblock(int nSockFd)
{
	int ret = -1;
#ifdef _WIN32
	int val = 1;
	if(ioctlsocket(nSockFd,FIONBIO,(unsigned long*)&val) != SOCKET_ERROR)
		ret = 0;
#else
    int flags;
    flags = fcntl(nSockFd, F_GETFL, 0);
    if (flags >= 0) {
#ifdef _AIX
    	flags |= FNONBLOCK;
#else
    	flags |= O_NONBLOCK;
#endif
    	ret = fcntl(nSockFd, F_SETFL, flags);
	}
#endif
	return ret;
}

int InetSocket::UnsetNonblock(int nSockFd)
{
	int ret = -1;
#ifdef _WIN32
	int val = 0;
	ret = ioctlsocket(nSockFd,FIONBIO,(unsigned long*)&val);
#else
    int flags;
    flags = fcntl(nSockFd, F_GETFL, 0);
    if (flags < 0) return -1;
#ifdef _AIX
    flags &= ~FNONBLOCK;
#else
    flags &= ~O_NONBLOCK;
#endif
    ret = fcntl(nSockFd, F_SETFL, flags);
#endif
	return ret;
}

// added by kskim. 2008.9.9 (for sms-agent)
int InetSocket::Select(int nSockFd,fd_set *rs,fd_set *ws,fd_set *es,int nSec)
{
    struct timeval  tval;
    int             ret;

	if(nSec <= 0) return nSockFd;

	if(nSockFd >= 0) {
		if(rs) {
			FD_ZERO(rs);
			FD_SET(nSockFd, rs);
		}
		if(ws) {
			FD_ZERO(ws);
			FD_SET(nSockFd, ws);
		}
		if(es) {
			FD_ZERO(es);
			FD_SET(nSockFd, es);
		}

    	tval.tv_sec     = nSec;
    	tval.tv_usec    = 0;

    	ret = select(nSockFd + 1, rs, ws, es, nSec ? &tval : NULL);
	//	printf("select return : %d, fd: %d, error : %d\n", ret, nSockFd, errno);

    	if (ret <= 0) return ret;
    	if (rs && FD_ISSET(nSockFd, rs)) return ret;
    	if (ws && FD_ISSET(nSockFd, ws)) return ret;
    	if (es && FD_ISSET(nSockFd, es)) return ret;
	}

    return -1;
}

// modified by kskim(2008.9.9)
int InetSocket::Select(int nSockFd,int nSec)
{
    fd_set          rset;
    fd_set          wset;
    return  Select(nSockFd, &rset, &wset, NULL, nSec);
}

int InetSocket::SetTimeout(int nSecs)
{
	m_nTimeout = nSecs;
	return 1;
}

int InetSocket::GetTimeout()
{
	return m_nTimeout;
}

void InetSocket::StartTime()
{
#ifdef _WIN32
	m_tv1 = GetTickCount();
#else
	gettimeofday(&m_tv1,NULL);
#endif
}

int InetSocket::EndTime()
{
	int ts = 0;
#ifdef _WIN32
	m_tv2 = GetTickCount();
	ts = m_tv2 - m_tv1;
#else
	struct timeval tv;
	gettimeofday(&m_tv2,NULL);
	tv.tv_sec = m_tv2.tv_sec - m_tv1.tv_sec;
	tv.tv_usec = m_tv2.tv_usec - m_tv1.tv_usec;
	if(tv.tv_usec < 0) {
		--tv.tv_sec;
		tv.tv_usec += 1000000;
	}
	ts = 1000*tv.tv_sec + (long)(tv.tv_usec/1000.0);
#endif
	return ts;
}

int InetSocket::Send(int fd,char* msg,int size,int opt)
{
	fd_set fdvar;
    struct timeval tv;
    if(m_nTimeout > 0) {
        FD_ZERO(&fdvar);
        FD_SET((unsigned int)m_nSockFd,&fdvar);
        tv.tv_sec = (int)(m_nTimeout);
        tv.tv_usec = 0;
    }

    int nwritten = 0;
    int nleft = size;
    while(nleft > 0) {
		if(m_nTimeout > 0) {
            if(select(m_nSockFd+1,NULL,&fdvar,NULL,&tv) <= 0) return 0;
        }

		nwritten = send(fd,msg,nleft,opt);

#ifdef _KSKIM
	   	if(nwritten <= 0) return nwritten;
#else
	   	if(nwritten < 0) {
		//	TRACE("TCPSOCK : Network down\n"); // added by kskim. 2007-07-20
			return nwritten;
		} else if(nwritten == 0) {
			//TRACE("TCPSOCK : Time out\n"); // added by kskim. 2007-07-20
			return nwritten;
		}
#endif
	
		nleft -= nwritten;
	   	msg += nwritten;
    }
    
    return (size-nleft);
}

int InetSocket::Send(char* msg, int size,int opt)
{
    if(m_nSockFd <= 0) return -1;
    return Send(m_nSockFd,msg,size,opt);
}

int InetSocket::Send(int msg,int opt)
{
    msg = htonl(msg);
    return Send((char*)&msg,sizeof(msg),opt);
}

int InetSocket::Send(float msg,int opt)
{
    msg = htonf(msg);
    return Send((char*)&msg,sizeof(msg),opt);
}


int InetSocket::SendAsync(int fd,char* msg,int size,int opt)
{
	if(m_nTimeout > 0) {
        fd_set fdvar;
        struct timeval tv;

        FD_ZERO(&fdvar);
        FD_SET((unsigned int)m_nSockFd,&fdvar);
        tv.tv_sec = (int)(m_nTimeout);
        tv.tv_usec = 0;

        if(select(m_nSockFd+1,NULL,&fdvar,NULL,&tv) <= 0)
            return 0;
    }
	return send(fd,msg,size,opt);
}

int InetSocket::SendAsync(char* msg, int size,int opt)
{
    if(m_nSockFd <= 0) return -1;
    return SendAsync(m_nSockFd,msg,size,opt);
}

int InetSocket::Recv(int fd,char* msg,int size,int opt)
{
	// initialize memory
	memset(msg,0,size);			// 2005.11.09.

	fd_set fdvar;
    struct timeval tv;
    if(m_nTimeout > 0) {
        FD_ZERO(&fdvar);
        FD_SET((unsigned int)m_nSockFd,&fdvar);
        tv.tv_sec = (int)(m_nTimeout);
        tv.tv_usec = 0;
    }

    int nread = 0;
    int nleft = size;
    while(nleft > 0) {
		if(m_nTimeout > 0) {
            if(select(m_nSockFd+1,&fdvar,NULL,NULL,&tv) <= 0) return 0;
        }

		nread = recv(fd,msg,nleft,opt);
#ifdef _KSKIM
	   	if(nread <= 0) return nread;
#else
		if(nread < 0) { 
			//TRACE("TCPSOCK: Network down\n"); // added by kskim. 2007-07-20
			return nread;
		} else if (nread == 0) {
			//TRACE("TCPSOCK: Timeout\n"); // added by kskim. 2007-07-20
			return nread;
		}
#endif
	
		nleft -= nread;
	   	msg += nread;
    }
    
    if(nleft > 0) return 0;
    return size; 
}

int InetSocket::Recv(char* msg, int size,int opt)
{
    if(m_nSockFd <= 0) return -1;
    return Recv(m_nSockFd,msg,size,opt);
}

int InetSocket::Recv(int& msg,int opt)
{
    int ret = Recv((char*)&msg,sizeof(msg),opt);
    msg = ntohl(msg);
    return ret;
}

int InetSocket::Recv(float& msg,int opt)
{
    int ret = Recv((char*)&msg,sizeof(msg),opt);
    msg = ntohf(msg);
    return ret;
}

int InetSocket::RecvRaw(char* msg, int size,int opt)
{
    if(m_nSockFd <= 0) return -1;
    return Recv(m_nSockFd,msg,size,opt);
}

int InetSocket::RecvRaw(int& msg,int opt)
{
	int ret;
    ret = RecvRaw((char*)&msg,sizeof(msg),opt);
    msg = ntohl(msg);
    return ret;
}

int InetSocket::RecvAsync(int fd,char* msg,int size,int opt)
{
	// initialize memory
	memset(msg,0,size);			// 2005.11.09.

	if(m_nTimeout > 0) {
        fd_set fdvar;
        struct timeval tv;
		int selfd;

        FD_ZERO(&fdvar);
        FD_SET((unsigned int)m_nSockFd,&fdvar);
        tv.tv_sec = (int)(m_nTimeout);
        tv.tv_usec = 0;

		selfd = select(m_nSockFd+1,&fdvar,NULL,NULL,&tv);
		if(selfd <= 0) return 0;
    }
	return recv(fd,msg,size,opt);
}

int InetSocket::RecvAsync(char* msg, int size, int opt)
{
    if(m_nSockFd <= 0) return -1;
    return InetSocket::RecvAsync(m_nSockFd,msg,size,opt);
}

unsigned int InetSocket::GetPeerAddr(int fd)
{
	struct sockaddr_in	sin;
	mysocklen_t			sin_len = sizeof(sin);
	if(fd == 0) fd = m_nSockFd;
	if(getpeername(fd,(struct sockaddr*)&sin,&sin_len) < 0) return 0;
	return ntohl(sin.sin_addr.s_addr); 	// Host Byte Order
}

#ifndef _USE_IPV4ONLY
unsigned int InetSocket::GetPeerAddr(struct in6_addr *sin6_addr, int fd)
{    
    struct sockaddr	ss;
    mysocklen_t			ss_len = sizeof(ss);
	if(fd == 0) fd = m_nSockFd;
	if(getpeername(fd,(struct sockaddr*)&ss,&ss_len) < 0) return 0;
	
	if(ss.sa_family == AF_INET) {
		return ntohl( ((struct sockaddr_in*)&ss)->sin_addr.s_addr ); 	// Host Byte Order
	} else {
		if(sin6_addr) {
			memcpy(sin6_addr, &((struct sockaddr_in6*)&ss)->sin6_addr, sizeof(struct in6_addr));
			return 1;
		}
	}
	return 0;
}
#endif
int InetSocket::GetSockName(char* addr,int size,int& port,int fd)
{
    struct sockaddr_in  sin;
    mysocklen_t         sin_len = sizeof(sin);

    if(!addr) return -1;

    if(fd == 0) fd = m_nSockFd;
    if (getsockname(fd, (struct sockaddr *) &sin, &sin_len) < 0)
    return -1;

#if     (_HPUX || _SOLARIS || _OSF1==4)
    strncpy(addr, inet_ntoa(sin.sin_addr), size);
#else
    myinet_ntop(AF_INET, &sin.sin_addr, addr, size);
#endif
    port = ntohs(sin.sin_port);

    return 0;
}

int InetSocket::GetPeerName(char* addr,int size,int& port,int fd)
{
#ifdef _USE_IPV4ONLY
    struct sockaddr_in  sin;
    mysocklen_t         sin_len = sizeof(sin);

    if(!addr) return -1;

    if(fd == 0) fd = m_nSockFd;
    if(getpeername(fd,(struct sockaddr*)&sin,&sin_len) < 0) return -1;

#if     (_WIN32 || _HPUX || _SOLARIS || _OSF1==4)
    strncpy(addr, inet_ntoa(sin.sin_addr), size);
#else
    inet_ntop(AF_INET, &sin.sin_addr, addr, size);
#endif
    port = ntohs(sin.sin_port);
#else // #ifdef _USE_IPV4ONLY
    struct sockaddr	ss;
    mysocklen_t			ss_len = sizeof(ss);

	if(!addr) return -1;

	if(fd == 0) fd = m_nSockFd;
	if(getpeername(fd,(struct sockaddr*)&ss,&ss_len) < 0) return -1;
	//
	union {
		struct sockaddr     *sa;
		struct sockaddr_in  *sa_in;
		struct sockaddr_in6 *sa_in6;
	} u;
	u.sa = &ss;
	if (ss.sa_family == AF_INET) {
		myinet_ntop(ss.sa_family, &(u.sa_in->sin_addr), addr, size);
		port = (int) ntohs(u.sa_in->sin_port);
	} else {
		myinet_ntop(ss.sa_family, &(u.sa_in6->sin6_addr), addr, size);
		port = (int) ntohs(u.sa_in6->sin6_port);
	}
#endif
	return 0;
}


void InetSocket::Close(int fd)
{
    if(fd > 0) {
#ifdef _WIN32
		closesocket(fd);
#else
		close(fd);
#endif
    } else if(m_nSockFd > 0) {
#ifdef _WIN32
		closesocket(m_nSockFd);
#else
		close(m_nSockFd);
#endif
	   	m_nSockFd = -1;
	}
	// reset header & sequence number
	memset(&m_msgHead,0,sizeof(m_msgHead));
	m_uSendSeq = 0;
	m_uRecvSeq = 0;
}

void InetSocket::SetSocket(int fd)
{
    m_nSockFd = fd;
}

void InetSocket::SetHost(char* pHost,int nPort)
{
	strncpy(m_szHost,pHost,sizeof(m_szHost)-1);
	m_nPort = nPort;
}

char* InetSocket::GetDataBuf(int size)
{
	// added by hjson 2008-07-02
	if(size > TCPSOCK_LIMITBUF) {
		fprintf(stderr,"InetSocket::GetDataBuf over max limit %d\n",size);
		return NULL;
	}

    // mod by shnoh 2004.7.5
	if(m_nDataBuf >= size && m_nDataBuf < TCPSOCK_LIMITBUF) 
		return  m_pDataBuf;

    if(m_nDataBuf < size) {
        m_nDataBuf = size + 1024;
        m_pDataBuf = (char*)realloc(m_pDataBuf,m_nDataBuf);
    }
    return m_pDataBuf;
}

char* InetSocket::GetSendBuf(int size)
{
	// added by hjson 2008-07-02
	if(size > TCPSOCK_LIMITBUF) {
		fprintf(stderr,"InetSocket::GetSendBuf over max limit %d\n",size);
		return NULL;
	}

    // added by shnoh 2004.7.5
	if(m_nSendBuf >= size && m_nSendBuf < TCPSOCK_LIMITBUF)
		return  m_pSendBuf;

    if(m_nSendBuf < size) {
        m_nSendBuf = size + 1024;
        m_pSendBuf = (char*)realloc(m_pSendBuf,m_nSendBuf);
    }
    return m_pSendBuf;
}

char* InetSocket::GetRecvBuf(int size)
{
	// added by hjson 2008-07-02
	if(size > TCPSOCK_LIMITBUF) {
		fprintf(stderr,"InetSocket::GetRecvBuf over max limit %d\n",size);
		return NULL;
	}

    // added by shnoh 2004.7.5
	if(m_nRecvBuf >= size && m_nRecvBuf < TCPSOCK_LIMITBUF)
		return  m_pRecvBuf;

    if(m_nRecvBuf < size) {
        m_nRecvBuf = size + 1024;
        m_pRecvBuf = (char*)realloc(m_pRecvBuf,m_nRecvBuf);
    }
    return m_pRecvBuf;
}

void InetSocket::SendHeader(int msg,int sub)
{
	m_msgHead.id = TCPSOCK_IDENT;
	m_msgHead.ver = 0;
	m_msgHead.msg = msg;
	m_msgHead.sub = sub;
	m_msgHead.seqno = ++m_uSendSeq;
	m_nSendLen = sizeheader;
	GetSendBuf(m_nSendLen);
}

void InetSocket::SendSet(int val)
{
	GetSendBuf(m_nSendLen + sizeint);
	val = htonl(val);
	memcpy(&m_pSendBuf[m_nSendLen],&val,sizeint);
	m_nSendLen += sizeint;
}

void InetSocket::SendSet(unsigned int val)
{
	GetSendBuf(m_nSendLen + sizefloat);
	val = htonl(val);
	memcpy(&m_pSendBuf[m_nSendLen],&val,sizefloat);
	m_nSendLen += sizefloat;
}

void InetSocket::SendSet(long val)
{
	GetSendBuf(m_nSendLen + sizefloat);
	val = htonl(val);
	memcpy(&m_pSendBuf[m_nSendLen],&val,sizefloat);
	m_nSendLen += sizefloat;
}

void InetSocket::SendSet(float val)
{
	GetSendBuf(m_nSendLen + sizefloat);
	val = htonf(val);
	memcpy(&m_pSendBuf[m_nSendLen],&val,sizefloat);
	m_nSendLen += sizefloat;
}

void InetSocket::SendSet(double val)
{
	GetSendBuf(m_nSendLen + sizedouble);
	val = htond(val);
	memcpy(&m_pSendBuf[m_nSendLen],&val,sizedouble);
	m_nSendLen += sizedouble;
}

void InetSocket::SendSet(myint64 val)
{
	GetSendBuf(m_nSendLen + sizeint64);
	val = htonint64(val);
	memcpy(&m_pSendBuf[m_nSendLen],&val,sizeint64);
	m_nSendLen += sizeint64;
}

void InetSocket::SendSet(char* val,int size)
{
	GetSendBuf(m_nSendLen + size);
	memcpy(&m_pSendBuf[m_nSendLen],val,size);
	m_nSendLen += size;
}

// shnoh 2004.5.18
void InetSocket::SendSetStr(char *val,int size)
{
    int len;

	if(!val || size <=0 ) return;

	len = strlen(val);
	if(len >= size) {
		val[size - 1] = '\0';
		len = strlen(val);
	}

	SendSet(len);
	SendSet(val,len);
}	

int InetSocket::SendFlush(int opt)
{
    int ret = -1;
    if(!IsConnected()) return ret;

    if(m_nSendLen > 0 && m_msgHead.id != 0) {
       	// set data length
       	m_msgHead.len = m_nSendLen - sizeheader;
       	// set padding length
       	ret = m_nSendLen % TCPSOCK_SEGSIZE;
       	if(ret > 0) {
		    m_msgHead.padd = TCPSOCK_SEGSIZE - ret;
		    m_nSendLen += m_msgHead.padd;
		    GetSendBuf(m_nSendLen);
  		} else {
		    m_msgHead.padd = 0;
	   	}
	   	// copy head to the buffer
   		m_msgHead.hton();
	   	memcpy(m_pSendBuf,&m_msgHead,sizeheader);
	   	m_msgHead.ntoh();

		// send head + data
       	ret = Send(m_pSendBuf,m_nSendLen,opt);	// virutal
#ifdef _KSKIM
		if(ret <= 0) {							// for changmin
#else
		if(ret != m_nSendLen) {					// 2008.07.03 by kskim
			ret = -1;
#endif
			fprintf(stderr,
				"[InetSocket::SendFlush] msg %d: can't send, closed!! (%s:%d)\n",
				m_msgHead.msg,m_szHost,m_nPort);
			Close();					
		}

		// reset send length & flag
       	m_nSendLen = 0;
		m_msgHead.id = 0;						// 2005.04.09. added by hjson
    } else {
		fprintf(stderr,
			"[InetSocket::SendFlush] missing SendHeader, closed!! (%s:%d)\n",
			m_szHost,m_nPort);
		Close();
	}

    return ret;
}

int InetSocket::RecvHeader(int& msg,int opt)
{
    msg = 0;
    if(!IsConnected()) return -1;

    m_nRecvCur = m_nRecvLen = 0;
    int len, ret = Recv((char*)&m_msgHead,sizeheader,opt);
    if(ret == sizeheader) {
       	m_msgHead.ntoh(); 
       	// check seqno & id & head
       	if(m_msgHead.seqno==++m_uRecvSeq && m_msgHead.id==TCPSOCK_IDENT) {
			len = m_msgHead.len + m_msgHead.padd;
		    if(len > 0) {
		       	GetRecvBuf(len);
		       	// recv data
		       	int num = Recv(m_pRecvBuf,len);
		       	if(num == len) {
       				msg = m_msgHead.msg;
					m_nRecvLen = m_msgHead.len;
				} else {
					ret = -1;	
					Close();
					// 2005.04.09. added by hjson
					fprintf(stderr,
						"[InetSocket::RecvHeader] can't recv, closed!! (%s:%d)\n",
						m_szHost,m_nPort);
				}
	    	}
       	} else {
			ret = -1;
			Close();
			// 2005.04.09. added by hjson
			fprintf(stderr,
				"[InetSocket::RecvHeader] incorrect sequence, closed!! (%s:%d)\n",
				m_szHost,m_nPort);
		}
#ifdef _KSKIM
    } else if(ret <= 0) {
#else
    } else { // 2008.07.03 modified by kskim
		ret = -1;
#endif
		Close();
	}

    return ret;
}

void InetSocket::RecvLen(int& val,int size)
{
    val = 0;
    RecvGet(val);
    if(val < 0) {
        val = 0;
    } else if(val >= size) {
		val = 0;
	}
}

void InetSocket::RecvGet(int& val)
{
    val = 0;
    if(m_nRecvLen - m_nRecvCur >= sizeint) {
       	memcpy(&val,&m_pRecvBuf[m_nRecvCur],sizeint); val = ntohl(val);
       	m_nRecvCur += sizeint;
    }
}

void InetSocket::RecvGet(unsigned int& val)
{
    val = 0;
    if(m_nRecvLen - m_nRecvCur >= sizeint) {
       	memcpy(&val,&m_pRecvBuf[m_nRecvCur],sizeint); val = ntohl(val);
       	m_nRecvCur += sizeint;
    }
}

void InetSocket::RecvGet(long& val)
{
    val = 0;
    if(m_nRecvLen - m_nRecvCur >= sizeint) {
       	memcpy(&val,&m_pRecvBuf[m_nRecvCur],sizeint); val = ntohl(val);
       	m_nRecvCur += sizeint;
    }
}

void InetSocket::RecvGet(float& val)
{
    val = 0;
    if(m_nRecvLen - m_nRecvCur >= sizefloat) {
       	memcpy(&val,&m_pRecvBuf[m_nRecvCur],sizefloat); val = ntohf(val);
       	m_nRecvCur += sizefloat;
    }
}

void InetSocket::RecvGet(double& val)
{
    val = 0;
    if(m_nRecvLen - m_nRecvCur >= sizedouble) {
       	memcpy(&val,&m_pRecvBuf[m_nRecvCur],sizedouble); val = ntohd(val);
       	m_nRecvCur += sizedouble;
    }
}

void InetSocket::RecvGet(myint64& val)
{
    val = 0;
    if(m_nRecvLen - m_nRecvCur >= sizeint64) {
       	memcpy(&val,&m_pRecvBuf[m_nRecvCur],sizeint64); val = ntohint64(val);
       	m_nRecvCur += sizeint64;
    }
}

void InetSocket::RecvGet(char* val,int size)
{
	int	len=0;

	if(!val || size <= 0) return;
	memset(val,0,size);

    if(m_nRecvLen - m_nRecvCur >= size) {
       	memcpy(val,&m_pRecvBuf[m_nRecvCur],size);
       	m_nRecvCur += size;
    } else {
       	len = m_nRecvLen - m_nRecvCur;
       	memcpy(val,&m_pRecvBuf[m_nRecvCur],len);
       	m_nRecvCur += len;
    }
}

// shnoh 2004.5.18
void InetSocket::RecvGetStr(char *val, int size)
{
    int len, over;

	if(!val || size <= 0) return;

	memset(val,0,size);
	RecvGet(len);

	if(len < 0) {
		len = 0;

	} else if(len >= size) {

		over = len - (size - 1);
		len = size -1;
		RecvGet(val,len);
		m_nRecvCur += over;

		// check overflow
		if(m_nRecvCur < 0) {
			m_nRecvLen = 0;
			m_nRecvCur = 0;
		}

	} else {
		RecvGet(val,len);
	}
}

// added by kskim. 2008.07.03
void InetSocket::SetBacklog(int nBacklog) 
{
	m_nBacklog = nBacklog;
}

void InetSocket::GetFdLimit(int &hardlimit, int &curlimit)
{
#ifndef _WIN32
	struct rlimit rlp;
	if(getrlimit(RLIMIT_NOFILE, &rlp) != -1) {
		hardlimit = rlp.rlim_max;
		curlimit = rlp.rlim_cur;
	}
#endif
}

int InetSocket::SetFdLimit(int curlimit)
{
#ifndef _WIN32
	struct rlimit rlp;
	if(getrlimit(RLIMIT_NOFILE, &rlp) != -1) {
		if(rlp.rlim_max > (unsigned int)curlimit) {
			rlp.rlim_cur = curlimit;
			return setrlimit(RLIMIT_NOFILE, &rlp);
		}
	}
	return -1;
#else
	return 0;
#endif
}

int InetSocket::SetBroadcast()
{
    if(m_nSockFd < 0) return -1;

    int val = 1;
    if(setsockopt(m_nSockFd,SOL_SOCKET,SO_BROADCAST,(char*)&val,sizeof(val))<0) 
	{
        return -1;
    }

    return 0;
}

////////////////////////////////////////////////////////////
// utility functions
//

unsigned int IP2Num(char* ip)
{
	return ntohl(inet_addr(ip));	// network to host byte order
}

void Num2IP(unsigned int num,char* ip,int size)
{
	struct in_addr in;
	in.s_addr = htonl(num);			// host to network byte order
#ifdef _AIX
	myinet_ntop(AF_INET, &in, ip, size-1);
#else
	strncpy(ip,inet_ntoa(in),size-1);
#endif
}

void Num2IP(int num,char* ip,int size)
{
	Num2IP((unsigned int)num,ip,size);
}

void Num2IP(long num,char* ip,int size)
{
	Num2IP((unsigned int)num,ip,size);
}

void Num2IP(unsigned long num,char* ip,int size)
{
	Num2IP((unsigned int)num,ip,size);
}

struct in_addr Num2InAddr(unsigned int num)
{
	struct in_addr in;
	in.s_addr = htonl(num);
	return in;
}

unsigned int InAddr2Num(struct in_addr& in)
{
	return ntohl(in.s_addr);		// network to host byte order
}

unsigned int SockAddrIn2Num(struct sockaddr_in& in)
{
	return ntohl(in.sin_addr.s_addr);
}

/*! 인자로 주어진 sockaddr를 숫자 형태로 변환하는 함수 */
unsigned int SockAddr2Num(struct sockaddr& in)
{
	struct sockaddr_in in2;
	memcpy(&in2,&in,sizeof(in2));
	return SockAddrIn2Num(in2);
}

int GetIPVersion(char *pIp)
{
	int ret = -1;
#ifdef _USE_IPV4ONLY
	if(strstr(pIp,":")) {
		ret = AF_INET6;
	} else {
		ret = AF_INET;
	}
#else
	int en;
	struct addrinfo hints, *res;
	memset(&hints, 0x00, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;  /* always return canonical name */
	hints.ai_family = 0;        /* 0, AF_INET, AF_INET6, etc. */
	hints.ai_socktype = 0;  /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

	if ( (en = getaddrinfo(pIp, NULL, &hints, &res)) != 0) {
		fprintf(stderr, "GetInetFamily():getaddrinfo error for %s: %s\n",
				pIp, gai_strerror(en));
		return -1;
	}
	ret = res->ai_family;
	freeaddrinfo(res);
#endif
	return ret;
}

