/*
 * TcpSock 2.0
 * Copyright (c) 2004 Brainzsquare, Inc.
 *
 * 2005.08.05. modified Connect() by changmin
 * 2007.07.20. modified Recv(), Send(), added TRACE()  by kskim
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

#include "tcpsock4.h"
#include "debug.h"

//////////////////////////
// CTcpSock

CTcpSock4::CTcpSock4(const char* pHost,Port nPort)
{
	// init connection
    Connect(pHost,nPort);
}

CTcpSock4::~CTcpSock4()
{
}

int CTcpSock4::Connect(const char* pHost,Port nPort,int nSecs,int nSndBuf,int nRcvBuf, const char *pszBindIp)
{
	if (!pHost || !strlen(pHost) || nPort<=0) return -1;

	// if already connected, return m_nSockFd
	if (m_nSockFd > 0) return m_nSockFd;

	// setup server address
	sockaddr_in dest;
	struct hostent* hp = NULL;
	unsigned long addr = 0;
	int val = 0;
	mysocklen_t len = sizeof(val);

	memset((char*)&dest,0,sizeof(dest));
	if ((addr=inet_addr(pHost)) == (unsigned long) INADDR_NONE) {
		hp = gethostbyname(pHost);
		if (!hp || !hp->h_addr) return -1;
	}
	if (hp) {
		memcpy(&(dest.sin_addr),hp->h_addr,hp->h_length);
		dest.sin_family = hp->h_addrtype;
	} else {
		dest.sin_addr.s_addr = addr;
		dest.sin_family = AF_INET;
	}
	dest.sin_port = htons(nPort);

	// make socket
	int nSockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (nSockFd <= 0) return -1;

	// added by kskim. 2008.9.9
	if(pszBindIp && strlen(pszBindIp)) {
		sockaddr_in bind_addr;
		memset(&bind_addr, 0, sizeof bind_addr);
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_addr.s_addr = inet_addr(pszBindIp);
		bind_addr.sin_port = htons(0);
		if(bind(m_nSockFd, (struct sockaddr*) &bind_addr, sizeof bind_addr)) {
			fprintf(stderr,"CTcpSock4::Connect(bind, %s)", pszBindIp);
			InetSocket::Close(nSockFd);
			return -1;
		}
	}

#if		(_WIN32)
    if (connect(nSockFd,(struct sockaddr*)&dest,sizeof(dest)) != 0) {
		InetSocket::Close(nSockFd);
		return -1;
    }
#else
	if (nSecs > 0) {
		struct linger lg = { 1, 3 }; // off : 0, on : 1, sec
		setsockopt(nSockFd,SOL_SOCKET,SO_LINGER,(char*)&lg,sizeof(lg));
		SetNonblock(nSockFd);	// set non-blocking
		if (connect(nSockFd, (struct sockaddr *)&dest, sizeof (dest)) < 0 
				&& errno != EINPROGRESS) {
			InetSocket::Close(nSockFd);
			return -1;
		}
		if (Select(nSockFd,nSecs)<=0) {
			InetSocket::Close(nSockFd);
			return -1;
		}
		if (getsockopt(nSockFd,SOL_SOCKET,SO_ERROR,(char*)&val,&len) < 0) {
			InetSocket::Close(nSockFd);
			return -1;
		}
		if (val) {
			InetSocket::Close(nSockFd);
			return -1;
		}
		UnsetNonblock(nSockFd);	// reset non-blocking
	} else {
		if (connect(nSockFd,(struct sockaddr*)&dest,sizeof(dest)) != 0) {
			InetSocket::Close(nSockFd);
			return -1;
		}
	}
#endif // _WIN32

	// Set Socket;	2005.09.27.
	m_nSockFd = nSockFd;

	// make socket options
#ifdef _HJSON // 2008.10.6
	if(nSndBuf>0) {
		val = nSndBuf;
		setsockopt(m_nSockFd,SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(int));
	}
	if(nRcvBuf>0) {
		val = nRcvBuf;
		setsockopt(m_nSockFd,SOL_SOCKET,SO_RCVBUF,(char*)&val,sizeof(int));
	}
#else
	// send buffer
	val = 0; len = sizeof(val);
	if(getsockopt(m_nSockFd,SOL_SOCKET,SO_SNDBUF,(char*)&val,&len)==0
			&& nSndBuf > 0 && nSndBuf > val)
	{
		val = nSndBuf;
		setsockopt(m_nSockFd,SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(int));
	}
	// recv buffer
	val = 0; len = sizeof(val);
	if(getsockopt(m_nSockFd,SOL_SOCKET,SO_RCVBUF,(char*)&val,&len)==0
			&& nRcvBuf > 0 && nRcvBuf > val)
	{
		val = nRcvBuf;
		setsockopt(m_nSockFd,SOL_SOCKET,SO_RCVBUF,(char*)&val,sizeof(int));
	}
#endif
	val = 1;
	setsockopt(m_nSockFd,SOL_SOCKET,SO_REUSEADDR,(char*)&val,len);
	val = 1;
	setsockopt(m_nSockFd,SOL_SOCKET,SO_KEEPALIVE,(char*)&val,len);
#ifdef _AIX // added by kskim. 2008.04.08
	val = 1;
	setsockopt(m_nSockFd,SOL_SOCKET,TCP_NODELAY,(char*)&val,len);
#endif

	// cache host, port
	strncpy(m_szHost,pHost,sizeof(m_szHost)-1);
	m_nPort = nPort;

	return m_nSockFd;
}

int CTcpSock4::ConnectDns(const char* pHost,char* pIp,Port nPort,int& nDns)
{
	if((nDns=ResolvDns(pHost,pIp)) < 0) return -1;
	return Connect(pIp,nPort);
}


int CTcpSock4::ResolvDns(const char* pHost,char* pIp,sockaddr_in* pAddr)
{
    if(!pHost || !strlen(pHost)) return -1;

    // setup server address
    struct hostent *hp = NULL;
    unsigned long addr = 0;

    int nDns = 0;
    if((addr=inet_addr(pHost)) == (unsigned long)INADDR_NONE) {
#ifdef _WIN32
        DWORD tv, tv2;
	    tv = GetTickCount();
#else
        struct timeval tv,tv2;
	    gettimeofday(&tv,NULL);
#endif
        hp = gethostbyname(pHost);
        if(!hp || !hp->h_addr) return -1;
#ifdef _WIN32
	    tv2 = GetTickCount();
	    nDns = tv2 - tv;
#else
	    gettimeofday(&tv2,NULL);
    	tv.tv_sec = tv2.tv_sec - tv.tv_sec;
    	tv.tv_usec = tv2.tv_usec - tv.tv_usec;
    	if(tv.tv_usec < 0) {
      		--tv.tv_sec;
			tv.tv_usec += 1000000;
		}
	    nDns = 1000*tv.tv_sec + (long)(tv.tv_usec/1000.0);
#endif
    }
    if(hp) {
	    struct in_addr address;
	    memcpy(&address, hp->h_addr, sizeof(address));
	    if(pIp) {
#ifdef _AIX
			inet_ntop(AF_INET, &address, pIp,31);
#else
			strncpy(pIp,inet_ntoa(address),31);
#endif
		}
		if(pAddr) {
			memcpy(&(pAddr->sin_addr),hp->h_addr,hp->h_length);
			pAddr->sin_family = hp->h_addrtype;
		}
    } else {
		if(pIp) strcpy(pIp,pHost);
		if(pAddr) {
			pAddr->sin_addr.s_addr = addr;
			pAddr->sin_family = AF_INET;
		}
    }

	return nDns;
}

//////////////////////////
// STcpSock

STcpSock4::STcpSock4()
{
	m_nBacklog = 128;
}

STcpSock4::~STcpSock4()
{
    if(m_nSockFd>0) Close();
}

int STcpSock4::Open(Port prt,char* pIp)
{
    /* Create the socket. */
    m_nSockFd = socket(AF_INET,SOCK_STREAM,0);
    if(m_nSockFd < 0) return -1;

    int one;
    one = 1;
    setsockopt(m_nSockFd,SOL_SOCKET,SO_REUSEADDR,(char*)&one,sizeof(one));
#ifdef _AIX
    one = 1;
    setsockopt(m_nSockFd,IPPROTO_TCP,TCP_NODELAY,(char*)&one,sizeof(one));
#endif

    /* bind socket to specified address */
    sockaddr_in	svr_addr;
    memset((char *)&svr_addr, 0, sizeof(struct sockaddr_in));
    svr_addr.sin_family = AF_INET;
	if(pIp) svr_addr.sin_addr.s_addr = inet_addr(pIp);
	else svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    svr_addr.sin_port = htons (prt);
    if(bind(m_nSockFd,(struct sockaddr *)&svr_addr,sizeof(svr_addr)) < 0) {
#ifndef _WIN32
       	perror("STcpSock4: bind");
#endif
		Close();
		return -1;
    }
    if(listen(m_nSockFd, m_nBacklog) < 0) {		// for changmin
#ifndef _WIN32
       	perror("STcpSock4: listen");
#endif
		Close();
		return -1;
    }

    return m_nSockFd;
}

// assume : there are some input on server socket
int STcpSock4::Accept(char* pHost,int* pPort, int nSize)
{
	if(m_nSockFd < 0) {
		printf("STcpSSL::Accept Socket Descriptor closed!!\n");
		return -1;
	}
	
    /* Socket request on original socket */
	fd_set rset;
	struct timeval tm;
	int selfd = 0, newsockfd = -1;

	while(m_nSockFd>=0 && selfd==0) {
		FD_ZERO(&rset);
		FD_SET(m_nSockFd,&rset);
		tm.tv_sec = 1; tm.tv_usec = 0;	// 1 seconds
		selfd = select(m_nSockFd+1,&rset,NULL,NULL,&tm);
		if(selfd < 0) return -1;

		if(m_nSockFd>=0 && selfd>0 && FD_ISSET(m_nSockFd,&rset)) {
			struct sockaddr_in addr;
			int len = sizeof(addr);
    		newsockfd = accept(m_nSockFd,(sockaddr*)&addr,(mysocklen_t*)&len);
			if(newsockfd >= 0) {
				if(pHost) {
#ifdef _AIX
					inet_ntop(AF_INET, &addr.sin_addr, pHost, nSize);
#else
					strcpy(pHost,inet_ntoa(addr.sin_addr));
#endif
				}
				if(pPort) *pPort = ntohs(addr.sin_port);
				break;
			} 
			// added by kskim (req. hjson) 2006.3.15
			else {
#ifndef _WIN32
				int err = errno;
				if(err == EMFILE) {
					usleep(100000);
					return 0;
				}
#endif
			}

			// added by shnoh 2004.10.19
			if(errno != EINTR) Close();
        	return -1;
		}
    }

	if(pHost && strcmp(pHost,"127.0.0.1")) {
#ifdef _HJSON // 2008.10.6
    	int val;
	    val = TCPSOCK_MAXBUF;
	    setsockopt(newsockfd,SOL_SOCKET,SO_RCVBUF,(char*)&val,sizeof(val));
	    val = TCPSOCK_MAXBUF;
	    setsockopt(newsockfd,SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(val));
#else
		int val;
		mysocklen_t len;
		val = 0; len = sizeof(val);
		if(getsockopt(m_nSockFd,SOL_SOCKET,SO_RCVBUF,(char*)&val,&len)==0
				&& val < TCPSOCK_MAXBUF)
		{
			val = TCPSOCK_MAXBUF;
			setsockopt(newsockfd,SOL_SOCKET,SO_RCVBUF,(char*)&val,sizeof(val));
		}
		val = 0; len = sizeof(val);
		if(getsockopt(m_nSockFd,SOL_SOCKET,SO_SNDBUF,(char*)&val,&len)==0
				&& val < TCPSOCK_MAXBUF)
		{
			val = TCPSOCK_MAXBUF;
			setsockopt(newsockfd,SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(val));
		}
#endif

#ifdef _AIX
		val = 1;
		setsockopt(m_nSockFd,IPPROTO_TCP,TCP_NODELAY,(char*)&val,sizeof(val));
#endif
#ifdef _HJSON
		struct linger	lg  = { 1, 3}; // linger n, sec: 3
		setsockopt(m_nSockFd, SOL_SOCKET, SO_LINGER, (char *) &lg, sizeof lg);
#endif
	}

    return newsockfd;
}

/////////////////////////////////////////////////////
// UDP socket

CUdpSock4::CUdpSock4()
{
	m_nTimeout = 0;
}

CUdpSock4::~CUdpSock4()
{
    if(m_nSockFd>0) Close(m_nSockFd);
}

int CUdpSock4::Open(Port nPort,char* pIp)
{
    m_nSockFd = socket(AF_INET,SOCK_DGRAM,0);
    if(m_nSockFd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
	if(pIp) addr.sin_addr.s_addr = inet_addr(pIp);
	else addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(nPort);

    if(bind(m_nSockFd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
    {
#ifdef _WIN32
        closesocket(m_nSockFd);
#else
		close(m_nSockFd);
#endif
        m_nSockFd = -1;
        return -1;
    }

    return m_nSockFd;
}

int CUdpSock4::SendTo(char* msg,int size,char* ip,Port port,int opt)
{
    if(m_nSockFd < 0) return -1;

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));

    addr.sin_family = AF_INET;
    if(ip == NULL) addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    else addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    int ret = 0;
    ret = sendto(m_nSockFd,msg,size,opt,(sockaddr*)&addr,sizeof(addr));

    return ret;
}

int CUdpSock4::SendAddr(char* msg,int size,sockaddr* addr,mysocklen_t addrlen,int opt)
{
    if(m_nSockFd < 0) return -1;

    int ret = 0;
    ret = sendto(m_nSockFd,msg,size,opt,addr,addrlen);

    return ret;
}

int CUdpSock4::RecvFrom(char* msg,int size,sockaddr* addr,mysocklen_t* addrlen,int opt)
{
	if(m_nSockFd < 0) return -1;

    int ret = 0;
	fd_set rset;
	struct timeval tv;
	if(m_nTimeout > 0) {
		FD_ZERO(&rset);
		FD_SET((unsigned int)m_nSockFd,&rset);
		tv.tv_sec = (int)(m_nTimeout);
		tv.tv_usec = 0;
		ret = select(m_nSockFd+1,&rset,NULL,NULL,&tv);
		if(ret <= 0) return ret;
	}
   	ret = recvfrom(m_nSockFd,msg,size,opt,addr,addrlen);

    return ret;
}

