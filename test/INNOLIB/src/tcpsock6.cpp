/*
 * TcpSock 2.0
 * Copyright (c) 2010 Brainzsquare, Inc.
 *
 * 2010.11.19. IPv6 지원 by kskim
 * - get_in_port 함수 GetSockAddrPort 으로 이름 변경 하고 전역함수로 수정
 * - SockAddr2IP() 전역함수 추가
 * 2010.11.02. IPv6 지원 by lordang, kskim
 * - Open, Accept, Connect, ResolvDns, ConnectDns, GetSockName, GetPeerName,
 	 GetPeerAddr 함수 수정
 * - get_in_addr(), get_in_port(), 윈도우용 inet_ntop(), inet_pton() 함수 추가
 * 2007.07.20. modified Recv(), Send(), added TRACE()  by kskim
 * 2005.08.05. modified Connect() by changmin
 */

#include "stdafx.h"

#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef _AIX
#include <strings.h>
#endif

#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
//#include <net/if.h> // by lordang
#ifdef _AIX
#include <netinet/in.h>		// FOR KORNET AIX
#include <netinet/ip.h>		// FOR KORNET AIX
#endif
#ifndef _UNIXWARE
#include <netinet/tcp.h>
#endif
#endif

#include "tcpsock6.h"
#include "debug.h"

void *get_in_addr(struct sockaddr *sa)
{
	union {
		struct sockaddr     *sa;
		struct sockaddr_in  *sa_in;
		struct sockaddr_in6 *sa_in6;
	} u;
	u.sa = sa;
	if (sa->sa_family == AF_INET) {
		return &(u.sa_in->sin_addr);
	} else {
		return &(u.sa_in6->sin6_addr);
	}
}

//////////////////////////
// CTcpSock6

CTcpSock6::CTcpSock6(const char* pHost,Port nPort)
{
	// init connection
    Connect(pHost,nPort);
}

CTcpSock6::~CTcpSock6()
{
}

int CTcpSock6::Connect(const char* pHost,Port nPort,int nSecs,int nSndBuf,int nRcvBuf, const char *pszBindIp)
{
	if (!pHost || !strlen(pHost) || nPort<=0) return -1;

	// if already connected, return m_nSockFd
	if (m_nSockFd > 0) return m_nSockFd;

	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int nSockFd = 0;
	int val = 0;
	mysocklen_t len = sizeof(val);

	char szPort[30];
	mysnprintf(szPort, sizeof(szPort), "%d", nPort);

	memset(&hints, 0x00, sizeof(struct addrinfo));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if( getaddrinfo(pHost, szPort, &hints, &result) != 0 ) {
		fprintf(stderr, (char*) "CTcpSock6: getaddrinfo");
		return -1;
	}

	for(rp = result ; rp != NULL; rp=rp->ai_next) {
		if(rp->ai_family == AF_INET || rp->ai_family == AF_INET6) {
			nSockFd = Connect(rp, nSecs, pszBindIp);
			if(nSockFd >= 0) break;
		}
	}
	freeaddrinfo(result);
	if(nSockFd < 0) return nSockFd;

	// Set Socket;	2005.09.27.
	m_nSockFd = nSockFd;

	// make socket options
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
	val = 1;
	setsockopt(m_nSockFd,SOL_SOCKET,SO_REUSEADDR,(char*)&val,len);
	val = 1;
	setsockopt(m_nSockFd,SOL_SOCKET,SO_KEEPALIVE,(char*)&val,len);
	//
#ifdef _AIX // added by kskim. 2008.04.08
	val = 1;
	setsockopt(m_nSockFd,SOL_SOCKET,TCP_NODELAY,(char*)&val,len);
#endif
	// cache host, port
	strncpy(m_szHost,pHost,sizeof(m_szHost)-1);
	m_nPort = nPort;

	return m_nSockFd;
}

int CTcpSock6::Connect(addrinfo *ai, int nSecs, const char *pszBindIp)
{
	if(!ai) return -1;
	//
	sockaddr *pdest = ai->ai_addr;
	size_t destlen = ai->ai_addrlen;
	// socketopt
	int val = 0;
	mysocklen_t len = sizeof(val);

	// make socket
	int nSockFd = socket(ai->ai_family, SOCK_STREAM, 0);
	if (nSockFd <= 0) return -1;

	// added by kskim. 2008.9.9
	if(pszBindIp && strlen(pszBindIp)) {
		if(ai->ai_family ==  AF_INET6) {
			sockaddr_in6 bindaddr;
			memset(&bindaddr, 0, sizeof bindaddr);
			bindaddr.sin6_family = AF_INET6;
			inet_pton(AF_INET6, pszBindIp, (void*)&bindaddr.sin6_addr);
			bindaddr.sin6_port = htons(0);
			if(bind(m_nSockFd,(struct sockaddr*)&bindaddr,sizeof bindaddr)) {
				//TRACE("CTcpSock6::Connect(bind, %s)", pszBindIp);
				InetSocket::Close(nSockFd);
				return -1;
			}
		} else if(ai->ai_family ==  AF_INET) {
			sockaddr_in bindaddr;
			memset(&bindaddr, 0, sizeof bindaddr);
			bindaddr.sin_family = AF_INET;
			bindaddr.sin_addr.s_addr = inet_addr(pszBindIp);
			bindaddr.sin_port = htons(0);
			if(bind(m_nSockFd,(struct sockaddr*)&bindaddr,sizeof bindaddr)) {
				//TRACE("CTcpSock6::Connect(bind, %s)", pszBindIp);
				InetSocket::Close(nSockFd);
				return -1;
			}

		}
	}

#if		(_WIN32)
    if (connect(nSockFd,(struct sockaddr*)pdest,destlen) != 0) {
		InetSocket::Close(nSockFd);
		return -1;
    }
#else
	if (nSecs > 0) {
		struct linger lg = { 1, 3 }; // off : 0, on : 1, sec
		setsockopt(nSockFd,SOL_SOCKET,SO_LINGER,(char*)&lg,sizeof(lg));
		SetNonblock(nSockFd);	// set non-blocking
		if (connect(nSockFd, (struct sockaddr *)pdest, destlen) < 0 
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
		if (connect(nSockFd,(struct sockaddr*)pdest,destlen) != 0) {
			InetSocket::Close(nSockFd);
			return -1;
		}
	}
#endif // _WIN32

	return nSockFd;
}

int CTcpSock6::ConnectDns(const char* pHost,char* pIp,Port nPort,int& nDns)
{
	return Connect(pHost,nPort);
}

// ipv6.google.com, www.vsix.net
int CTcpSock6::ResolvDns(const char* pHost,char* pIp,sockaddr* pAddr,mysocklen_t* addrlen)
{
	if(!pHost || !strlen(pHost)) return -1;

    int nDns = 0;
	int	rc, fd;
	struct addrinfo	*res, *result, hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

#ifdef _WIN32
	DWORD tv, tv2;
	tv = GetTickCount();
#else
	struct timeval tv,tv2;
	gettimeofday(&tv,NULL);
#endif

	rc = getaddrinfo(pHost, NULL, &hints, &result);
	if (rc != 0) {
		printf("InetSocket::ResolvDns() getaddrinfo return code = %d (%s)\n"
				, rc, gai_strerror(rc));
		return 0;
	}
	printf("\n");

	for(res=result; res!=NULL; res = res->ai_next ) {
		//	printf("socket(family %d, socktype %d, protocol %d)", 
		//			res->ai_family,
		//			res->ai_socktype, res->ai_protocol);
		if(res->ai_family != AF_INET6 && res->ai_family != AF_INET) {
			continue;
		}
		/* Call socket() to make sure return values are valid */
		fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (fd < 0) {
			printf("call to socket() failed!\n");
		} else {
#ifdef _WIN32
			closesocket(fd);
#else
			close(fd);
#endif
			break;
		}
	}

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
	if(res) {
		if(pIp) {
			inet_ntop(res->ai_family, get_in_addr(res->ai_addr), pIp, 40);
		}
		if(pAddr) {
			memcpy(pAddr,res->ai_addr,res->ai_addrlen);
			*addrlen = res->ai_addrlen;
		}
	}
	freeaddrinfo(result);

	return nDns;

}

//////////////////////////
// STcpSock6

STcpSock6::STcpSock6()
{
	m_nBacklog = 128;
}

STcpSock6::~STcpSock6()
{
    if(m_nSockFd>0) Close();
}

int STcpSock6::Open(Port prt,char* pIp)
{
	mysocklen_t addrlen;
	char szPort[30];
	mysnprintf(szPort, sizeof(szPort), "%d", prt);

	int	on = 1, err; 
	struct addrinfo	hints, *res, *ressave;

	memset(&hints, 0, sizeof(struct addrinfo));	
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (err = getaddrinfo(pIp, szPort, &hints, &ressave)) != 0) {
		//TRACE((char*) "STcpSock6: Open (getaddrinfo error) for %s, %s: %s\n",
		//		pIp, szPort, gai_strerror(err));
	}

	for(res=ressave; res!=NULL; res = res->ai_next ) {
		/* Create the socket. */
		m_nSockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (m_nSockFd < 0)
			continue;		/* error, try next one */

		on = 1;
		setsockopt(m_nSockFd,SOL_SOCKET,SO_REUSEADDR,(char*)&on,sizeof(on));
#ifdef _AIX
		on = 1;
		setsockopt(m_nSockFd,IPPROTO_TCP,TCP_NODELAY,(char*)&on,sizeof(on));
#endif

		if(pIp && res->ai_family == AF_INET6)  
		{  
			on = 1;  
#ifdef _WIN32
			//TODO: IPv6 Windows IPV6ONLY Socket Option 처리
#else 
			//setsockopt(m_nSockFd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on));  
#endif
		}  

		if (bind(m_nSockFd, res->ai_addr, res->ai_addrlen) == 0) {
			break;			/* success */
		}

		Close(m_nSockFd);	/* bind error, close and try next one */
	}

    if(res)
        addrlen = res->ai_addrlen;	/* return size of protocol address */
	freeaddrinfo(ressave);

	if (res == NULL)	/* errno from final socket() or bind() */
	{
		//TRACE((char*)"STcpSock6: bind error for %s, %s\n", pIp, szPort);
		Close();
		return -1;
	}

    if(listen(m_nSockFd, m_nBacklog) < 0) {		// for changmin
		//TRACE((char*)"STcpSock6: listen error\n");
		Close();
		return -1;
    }

	return m_nSockFd;
}

// assume : there are some input on server socket
int STcpSock6::Accept(char* pHost, int* pPort, int size)
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
			struct sockaddr addr;
			//struct sockaddr_storage addr;
            // sizeof(sockaddr)은 16, sizeof(sockaddr_storage)은 128
            // len 이 16이면 IPv6값을 받을 수 없다.
            // sockaddr_storage 구조체가 정의되어 있지 않은 OS가 존재한다.
			mysocklen_t len = 127; 
    		newsockfd = accept(m_nSockFd, (struct sockaddr*)&addr,(mysocklen_t*)&len);
			if(newsockfd >= 0) {
				if(pHost) {	
					//inet_ntop(addr.ss_family, get_in_addr((struct sockaddr*)&addr), pHost, size);
					inet_ntop(addr.sa_family, get_in_addr(&addr), pHost, size);
				}
				if(pPort) {
					*pPort = GetSockAddrPort((struct sockaddr*)&addr);
				}
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

	if(pHost && strcmp(pHost,"127.0.0.1") && strcmp(pHost, "::1")) {
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

CUdpSock6::CUdpSock6()
{
	m_nTimeout = 0;
}

CUdpSock6::~CUdpSock6()
{
    if(m_nSockFd>0) Close(m_nSockFd);
}

int CUdpSock6::Open(Port nPort,char* pIp)
{
	mysocklen_t addrlen;
	char szPort[30];
	mysnprintf(szPort, sizeof(szPort), "%d", nPort);

	int	on = 1, err; 
	struct addrinfo	hints, *res, *ressave;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ( (err = getaddrinfo(pIp, szPort, &hints, &ressave)) != 0)
		fprintf(stderr, "CUdpSock6: Open (getaddrinfo error) for %s, %s: %s\n",
				pIp, szPort, gai_strerror(err));

	for(res=ressave; res!=NULL; res = res->ai_next ) {
		/* Create the socket. */
		m_nSockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (m_nSockFd < 0)
			continue;		/* error, try next one */

		if(pIp && res->ai_family == AF_INET6)  
		{  
			on = 1;
#ifdef _WIN32
			//TODO: IPv6 Windows IPV6ONLY Socket Option 처리
#else
			//setsockopt(m_nSockFd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on));
#endif
		}

		if (bind(m_nSockFd, res->ai_addr, res->ai_addrlen) == 0) {
			break;			/* success */
		}

		Close(m_nSockFd);	/* bind error, close and try next one */
	}

	if (res == NULL)	/* errno from final socket() or bind() */
	{
		fprintf(stderr, (char*) "CUdpSock6: bind error for %s, %s\n", pIp, szPort);
		freeaddrinfo(ressave);
		Close(m_nSockFd);
		m_nSockFd = -1;
		return -1;
	}

	addrlen = res->ai_addrlen;	/* return size of protocol address */
	freeaddrinfo(ressave);

	return m_nSockFd;
}

int CUdpSock6::SendTo(char* msg,int size,char* ip,Port port,int opt)
{
    //if(m_nSockFd < 0) return -1;

	char szIp[40];
	if(ip == NULL) {
		sprintf(szIp, "ff02::1"); // IPv6 Multicast all node group
	} else {
		strncpy(szIp, ip, sizeof(szIp));
	}
	
	char szPort[30];
	mysnprintf(szPort, sizeof(szPort), "%d", port);

	int	err; 
	struct addrinfo	hints, *res, *ressave;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ( (err = getaddrinfo(szIp, szPort, &hints, &ressave)) != 0)
		fprintf(stderr, "CUdpSock6: SendTo (getaddrinfo error) for %s, %s: %s\n",
				szIp, szPort, gai_strerror(err));

	for(res=ressave; res!=NULL; res = res->ai_next ) {
		//if(m_nSockFd) break;
		if(m_nSockFd) Close(m_nSockFd);

		/* Create the socket. */
		m_nSockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (m_nSockFd >= 0)
        {
			break;			/* success */
		}
	}

	if (res == NULL)	/* errno from final socket() or bind() */
	{
		fprintf(stderr, "CUdpSock6: socket error for %s, %s\n", szIp, szPort);
		//m_nSockFd = -1;
	}

	int ret = 0;
	ret = sendto(m_nSockFd,msg,size,opt,res->ai_addr,res->ai_addrlen);
	freeaddrinfo(ressave);

	//if(ret < 0) err = WSAGetLastError();

    return ret;
}

int CUdpSock6::SendAddr(char* msg,int size,sockaddr* addr,mysocklen_t addrlen,int opt)
{
    if(m_nSockFd < 0) return -1;

    int ret = 0;
    ret = sendto(m_nSockFd,msg,size,opt,addr,addrlen);

    return ret;
}

int CUdpSock6::RecvFrom(char* msg,int size,sockaddr* addr,mysocklen_t* addrlen,int opt)
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

void SockAddr2IP(struct sockaddr& in, char *ip, int size)
{
	inet_ntop(in.sa_family, get_in_addr((struct sockaddr*)&in), ip, size);
}

unsigned short GetSockAddrPort(struct sockaddr *sa)
{
	unsigned short port;
	union {
		struct sockaddr     *sa;
		struct sockaddr_in  *sa_in;
		struct sockaddr_in6 *sa_in6;
	} u;
	u.sa = sa;
	if (sa->sa_family == AF_INET)
		port = u.sa_in->sin_port;
	else
		port = u.sa_in6->sin6_port;
	return ntohs(port);
}

#ifdef _UDP_ECHO_S_TEST
int main(int argc,char* argv[])
{
#ifdef _WIN32   // startup socket; needed ws2_32.lib
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 2, 2 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )  return 0;
#endif

    struct sockaddr cliaddr;
    mysocklen_t addrlen;
	int sockfd, myport;
	char myip[40];
    char *pmyaddr = NULL;
	char buf[256];
	char *pIp = NULL;

	if(argc == 2) {
		myport = atoi(argv[1]);
	} else if(argc == 3) {
		pIp = argv[1];
		myport = atoi(argv[2]);
	} else {
		printf("Usage: %s [ <host> ] <service or port>\n", argv[0]);
		return 0;
	}

	CUdpSock6 sock;
	if(sock.Open(myport, pIp) < 0) {
		fprintf(stderr, "Open() Failed.(bind error)\n");
		return 0;
	}
	sock.SetTimeout(5);
	while(1) {
        int ret = sock.RecvFrom(buf, sizeof(buf), &cliaddr, &addrlen);
        if(ret>0) sock.SendAddr(buf, sizeof(buf), &cliaddr, addrlen);
        //sock.GetPeerName(myip, sizeof(myip), myport);
		if(ret>0) {
		inet_ntop(cliaddr.sa_family, get_in_addr((struct sockaddr*)&cliaddr), myip, sizeof(myip));
		myport = GetSockAddrPort((struct sockaddr*)&cliaddr);
        fprintf(stdout, "received from %s:%d\n", myip, myport);
		fprintf(stdout, "echo: <--%s\n", buf);
		}
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
    }
    sock.Close();
	return 0;

#ifdef _WIN32   // cleanup socket; needed ws2_32.lib
    WSACleanup();
#endif
}
#endif

#ifdef _UDP_ECHO_C_TEST
int main(int argc,char* argv[])
{
#ifdef _WIN32   // startup socket; needed ws2_32.lib
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 1, 1 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )  return 0;
#endif

	char buf[256];
	char myip[40];
	int myport;
	char *pIp = NULL;
	CUdpSock6 sock;

	if(argc == 2) {
		myport = atoi(argv[1]);
	} else if(argc == 3) {
		pIp = argv[1];
		myport = atoi(argv[2]);
	} else {
		printf("Usage: %s [ <host> ] <service or port>\n", argv[0]);
		return 0;
	}
#if 1
	if(sock.Open() < 0) {
		fprintf(stderr, "Open() Failed.(bind error)\n");
		return 0;
	}
#endif
	sock.SetTimeout(5);
	memset(buf, 0x00, sizeof(buf));
	fgets(buf, sizeof(buf), stdin);
	//read(0, buf, sizeof(buf));
    int ret = sock.SendTo(buf, sizeof(buf),pIp,myport);
    if(ret>0) ret = sock.RecvFrom(buf, sizeof(buf));
	if(ret>0) printf("-->%s", buf);
	sock.Close();

	return 0;

#ifdef _WIN32   // cleanup socket; needed ws2_32.lib
    WSACleanup();
#endif
}
#endif
