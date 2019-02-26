/*
 * CTcpSock4 
 * Copyright (c) 2010 Brainzsquare, Inc.
 */

#ifndef __TCPSOCK4_H__
#define __TCPSOCK4_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netinet/tcp.h>		// FOR KORNET AIX
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "inetsock.h"

//! Client tcp socket�� ���� Ŭ����
class CTcpSock4 : public InetSocket
{
public:
    CTcpSock4(const char* pHost=NULL,Port nPort=80);
    virtual ~CTcpSock4();

	/** �Է��� Client ������ �����ϱ� ���� �Լ�. 
	Connect: return value >= 0, if success, Timeout 30 secs	*/
    virtual int Connect(const char* pHost,Port nPort=80,int nSecs=30, int nSndBuf=8196, int RcvBuf = 8196, const char *pszBindIp = NULL); 
	/** �Է��� �ּ������� IP������ ��ȯ�Ͽ� �����ϱ� ���� �Լ��μ� DNS��ȯ�� 
	����� nDns������ Ȯ���� �� �ִ�.. Connect: return value >= 0*/
    int ConnectDns(const char* pHost,char* pIp,Port nPort,int& nDns);

	// ResolvDns: return value >= 0 if success
	static int ResolvDns(const char* pHost,char* pIp,sockaddr_in* pAddr=NULL);

};

// server tcp socket
//! �������� ����ϴ� TCP ���� Ŭ�����μ� ��Ʈ�� �����ϴ� ������ ����ؾ��Ұ�� ���ȴ�.
class STcpSock4 : public InetSocket
{
public:
    STcpSock4();
    virtual ~STcpSock4();

	//! ������ �����ϰ� �Է��� ��Ʈ�� �����Ѵ�. */
    int Open(Port prt,char* pIp=NULL);
	//! �����Լ��� accept ��Ȱ�� �ϴ� �Լ��̴�. */
    virtual int Accept(char* pHost=NULL,int* pPort=NULL, int nSize=16);
};

// udp socket
//! ����/Ŭ���̾�Ʈ���� ����ϴ� UDP ���� Ŭ���� 
class CUdpSock4 : virtual public InetSocket
{
public:
    CUdpSock4();
    virtual ~CUdpSock4();

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

#endif
