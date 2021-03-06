/*
 * CTcpSock 2.0 
 * Copyright (c) 2004 Brainzsquare, Inc.
 */

#ifndef __TCPSOCK6_H__
#define __TCPSOCK6_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifndef _UNIXWARE
#include <netinet/tcp.h>
#endif
#endif

#include "inetsock.h"

//! Client tcp socket을 위한 클래스
class CTcpSock6 : public InetSocket
{
public:
    CTcpSock6(const char* pHost=NULL,Port nPort=80);
    virtual ~CTcpSock6();

	// IPv6 & IPv4 
	int Connect(addrinfo *ai, int nSecs=30, const char *pszBindIp = NULL);

	/** 입력한 IP정보로 접속하기 위한 함수. 
	Connect: return value >= 0, if success, Timeout 30 secs	*/
    virtual int Connect(const char* pHost,Port nPort=80,int nSecs=30, int nSndBuf=8196, int RcvBuf = 8196, const char *pszBindIp = NULL); 


	/** 입력한 주소정보를 IP정보로 변환하여 접속하기 위한 함수로서 DNS변환된 
	결과를 nDns변수로 확인할 수 있다.. Connect: return value >= 0*/
    int ConnectDns(const char* pHost,char* pIp,Port nPort,int& nDns);

	// ResolveDns: return value >= 0 if success
	static int ResolvDns(const char* pHost,char* pIp,sockaddr* /*(struct sockaddr_storage*)*/ pAddr=NULL,mysocklen_t* addrlen=NULL);

	/*
	int 		 GetPeerName(char* addr,int size,int& port,int fd=0);
	unsigned int GetPeerAddr(int fd=0, struct in6_addr *sin6_addr=NULL);
	*/
};

// server tcp socket
//! 서버에서 사용하는 TCP 소켓 클래스로서 포트를 리슨하는 소켓을 사용해야할경우 사용된다.
class STcpSock6 : public InetSocket
{
public:
    STcpSock6();
    virtual ~STcpSock6();

	//! 소켓을 생성하고 입력한 포트를 리슨한다. */
    int Open(Port prt,char* pIp=NULL);
	//! 소켓함수중 accept 역활을 하는 함수이다. */
    virtual int Accept(char* pHost=NULL, int* pPort=NULL, int size=16); // modi for IPv6 by lordang
};

// udp socket
//! 서버/클라이언트에서 사용하는 UDP 소켓 클래스 
class CUdpSock6 : virtual public InetSocket
{
public:
    CUdpSock6();
    virtual ~CUdpSock6();

	//! 입력한 포트를 Binding한다.
    int Open(Port nPort=0,char* pIp=NULL);
	//! 소켓API함수중 sendto역활을 대신하는 함수
    int SendTo(char* msg,int size,char* ip,Port port,int opt=0);
	//! 소켓API함수중 sendto역활을 대신하는 함수
    int SendAddr(char* msg,int size,
		    sockaddr* addr,mysocklen_t addrlen,int opt=0);
	//! 소켓API함수중 recvfrom역활을 대신하는 함수
    int RecvFrom(char* msg,int size,
		    sockaddr* addr=NULL,mysocklen_t* addrlen=NULL,int opt=0);
};

/*! 인자로 주어진 sockaddr를 문자 형태로 변환하는 함수(IPv6지원을 위해서 40이상되어야 함 */
void			SockAddr2IP(struct sockaddr& in, char *ip, int size /*size > 40*/);
/*! 인자로 주어진 sockaddr 에서 Port정보를 리턴하는 함수(host byte order) */
unsigned short	GetSockAddrPort(struct sockaddr *sa);
//
unsigned int GetPeerAddr(int fd, struct in6_addr *sin6_addr);

#endif
