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

//! Client tcp socket�� ���� Ŭ����
class CTcpSock6 : public InetSocket
{
public:
    CTcpSock6(const char* pHost=NULL,Port nPort=80);
    virtual ~CTcpSock6();

	// IPv6 & IPv4 
	int Connect(addrinfo *ai, int nSecs=30, const char *pszBindIp = NULL);

	/** �Է��� IP������ �����ϱ� ���� �Լ�. 
	Connect: return value >= 0, if success, Timeout 30 secs	*/
    virtual int Connect(const char* pHost,Port nPort=80,int nSecs=30, int nSndBuf=8196, int RcvBuf = 8196, const char *pszBindIp = NULL); 


	/** �Է��� �ּ������� IP������ ��ȯ�Ͽ� �����ϱ� ���� �Լ��μ� DNS��ȯ�� 
	����� nDns������ Ȯ���� �� �ִ�.. Connect: return value >= 0*/
    int ConnectDns(const char* pHost,char* pIp,Port nPort,int& nDns);

	// ResolveDns: return value >= 0 if success
	static int ResolvDns(const char* pHost,char* pIp,sockaddr* /*(struct sockaddr_storage*)*/ pAddr=NULL,mysocklen_t* addrlen=NULL);

	/*
	int 		 GetPeerName(char* addr,int size,int& port,int fd=0);
	unsigned int GetPeerAddr(int fd=0, struct in6_addr *sin6_addr=NULL);
	*/
};

// server tcp socket
//! �������� ����ϴ� TCP ���� Ŭ�����μ� ��Ʈ�� �����ϴ� ������ ����ؾ��Ұ�� ���ȴ�.
class STcpSock6 : public InetSocket
{
public:
    STcpSock6();
    virtual ~STcpSock6();

	//! ������ �����ϰ� �Է��� ��Ʈ�� �����Ѵ�. */
    int Open(Port prt,char* pIp=NULL);
	//! �����Լ��� accept ��Ȱ�� �ϴ� �Լ��̴�. */
    virtual int Accept(char* pHost=NULL, int* pPort=NULL, int size=16); // modi for IPv6 by lordang
};

// udp socket
//! ����/Ŭ���̾�Ʈ���� ����ϴ� UDP ���� Ŭ���� 
class CUdpSock6 : virtual public InetSocket
{
public:
    CUdpSock6();
    virtual ~CUdpSock6();

	//! �Է��� ��Ʈ�� Binding�Ѵ�.
    int Open(Port nPort=0,char* pIp=NULL);
	//! ����API�Լ��� sendto��Ȱ�� ����ϴ� �Լ�
    int SendTo(char* msg,int size,char* ip,Port port,int opt=0);
	//! ����API�Լ��� sendto��Ȱ�� ����ϴ� �Լ�
    int SendAddr(char* msg,int size,
		    sockaddr* addr,mysocklen_t addrlen,int opt=0);
	//! ����API�Լ��� recvfrom��Ȱ�� ����ϴ� �Լ�
    int RecvFrom(char* msg,int size,
		    sockaddr* addr=NULL,mysocklen_t* addrlen=NULL,int opt=0);
};

/*! ���ڷ� �־��� sockaddr�� ���� ���·� ��ȯ�ϴ� �Լ�(IPv6������ ���ؼ� 40�̻�Ǿ�� �� */
void			SockAddr2IP(struct sockaddr& in, char *ip, int size /*size > 40*/);
/*! ���ڷ� �־��� sockaddr ���� Port������ �����ϴ� �Լ�(host byte order) */
unsigned short	GetSockAddrPort(struct sockaddr *sa);
//
unsigned int GetPeerAddr(int fd, struct in6_addr *sin6_addr);

#endif
