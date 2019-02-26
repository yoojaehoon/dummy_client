/*
 * InetSocket 
 * Copyright (c) 2011 Brainzsquare, Inc.
 */

#ifndef __INETSOCK_H__
#define __INETSOCK_H__

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
#endif

#ifndef INADDR_NONE
#define INADDR_NONE		(unsigned int)(-1)
#endif

// modified by kskim. 2009.3.4
#ifndef _MYSOCKLEN_T
#define _MYSOCKLEN_T
#if     (_UNIXWARE)
	typedef size_t		mysocklen_t;
#elif   (_WIN32 || _SOLARIS==5 || _SOLARIS==6 || _OSF1 || _HPUX)

#if !defined(_XOPEN_SOURCE_EXTENDED) // hpux
		typedef int			mysocklen_t;
#else
		typedef socklen_t 	mysocklen_t;
#endif

#else
	typedef socklen_t 	mysocklen_t;
#endif	// _UNIXWARE
#endif

#define TCPSOCK_MAXBUF		8192
#define TCPSOCK_MAX			64
#define TCPSOCK_SEGSIZE		128

#define TCPSOCK_IDENT		0x62382060
#define TCPSOCK_LIMITBUF	5048576     // 5M

/////////////////////////////////////////////
// integer 64 bits
#ifndef myint64
#ifdef _WIN32
typedef __int64				myint64;
typedef unsigned __int64	umyint64;
#else
typedef long long			myint64;
typedef unsigned long long	umyint64;
#endif
#endif

typedef unsigned short int Port;

const int sizeint = sizeof(int);
const int sizelong = sizeof(long);
const int sizefloat = sizeof(float);
const int sizedouble = sizeof(double);
const int sizeint64 = sizeof(myint64);

struct SMsgHeader {
	unsigned int	id;			// protocol id; don't set
	int				ver;		// packet version
	int				msg;		// main message; BSMSG_XXXX
	int				sub;		// sub message;
	int				len;		// data length; don't set;
	int				padd;		// the number of padding; don't set;
	int				user;		// user data
	unsigned int	seqno;		// seqno; don't set

	void hton()
	{
		id = htonl(id);
		ver = htonl(ver);
		msg = htonl(msg);
		sub = htonl(sub);
		len = htonl(len);
		padd = htonl(padd);
		user = htonl(user);
		seqno = htonl(seqno);
	}

	void ntoh()
	{
		id = ntohl(id);
		ver = ntohl(ver);
		msg = ntohl(msg);
		sub = ntohl(sub);
		len = ntohl(len);
		padd = ntohl(padd);
		user = ntohl(user);
		seqno = ntohl(seqno);
	}
};

const int sizeheader = sizeof(SMsgHeader);

//! ���ϻ���� ���� �ֻ��� Ŭ����
class InetSocket 
{
#ifdef _WIN32
	DWORD	m_tv1, m_tv2;
#else
	struct timeval	m_tv1, m_tv2;
#endif

protected:
    int		m_nSockFd;
    int		m_nTimeout;		// seconds; default 30 secs

	// data buffer (encoded)
	char*		m_pDataBuf;
	int			m_nDataBuf;

	// send buffer
	char*		m_pSendBuf;
	int			m_nSendBuf;
	int			m_nSendLen;

	// recv buffer
	char*		m_pRecvBuf;
	int			m_nRecvBuf;
	int			m_nRecvLen;
	int			m_nRecvCur;

	// protocol header
	SMsgHeader	m_msgHead;
	unsigned int m_uSendSeq;
	unsigned int m_uRecvSeq;

	// connection information
	char        m_szHost[64];
	int         m_nPort;
	int m_nBacklog;

public: 
    InetSocket(int s = -1) : m_nSockFd(s), m_nTimeout(30) {
		m_pDataBuf = NULL;
		m_nDataBuf = 0;

		m_pSendBuf = NULL;
		m_nSendBuf = 0;
		m_nSendLen = 0;

		m_pRecvBuf = NULL;
		m_nRecvBuf = 0;
		m_nRecvLen = 0;
		m_nRecvCur = 0;

		memset(&m_msgHead,0,sizeof(m_msgHead));
		m_uSendSeq = 0;
		m_uRecvSeq = 0;

		memset(m_szHost,0,sizeof(m_szHost));
		m_nPort = 0;
		m_nBacklog = 128;
	};
    virtual ~InetSocket() { 
		Close();
		if(m_pDataBuf) free(m_pDataBuf);
		if(m_pSendBuf) free(m_pSendBuf);
		if(m_pRecvBuf) free(m_pRecvBuf);
	};

    int Socket() { return m_nSockFd; }

	// set nonblock
    int SetNonblock(int nSockFd);
	// unset nonblock
	int UnsetNonblock(int nSockFd);
	// select
	int Select(int nSockFd,int nSec);
	int Select(int nSockFd, fd_set *rs, fd_set *ws, fd_set *es, int nSec);
	// timeout
	int SetTimeout(int nSec=30);	// default 30 seconds
	int GetTimeout();

	// start/end time to measure elapsed time (ms)
	void StartTime();
	int  EndTime();

	// ResolveDns: return value >= 0 if success
	static int ResolvDns(const char* pHost,char* pIp,sockaddr_in* pAddr=NULL);

	/** �Է°��� �����ϴ� �Լ�  return value > 0, if success */
    virtual int Send(int fd,char* msg,int size,int opt=0);
    virtual int Send(char*,int size,int opt=0);
	virtual int Send(int msg,int opt=0);
	virtual int Send(float msg,int opt=0);
	int 		SendRaw(char* msg, int size,int opt=0) { return Send(msg, size, opt); }
	int 		SendRaw(int msg,int opt=0) { return Send(msg, opt); }
	virtual int SendAsync(int fd,char* msg,int size,int opt=0);
	virtual int SendAsync(char* msg, int size,int opt=0);

	// recv
    virtual int Recv(int fd,char* msg,int size,int opt=0);
    virtual int Recv(char* msg,int size,int opt=0);
	virtual int Recv(int& msg,int opt=0);
	virtual int Recv(float& msg,int opt=0);
	int 		RecvRaw(char* buf,int size,int opt=0);
	int 		RecvRaw(int& msg,int opt=0);
	virtual int RecvAsync(int fd,char* msg,int size,int opt=0);
	virtual int RecvAsync(char* msg,int size,int opt=0);

	// packet protocol
	/** Zenius���� ����ϴ� Protocol���� ����ϴ� �Լ��μ� send-buffer�� ��������� 
	 �����ϴ� �Լ� �̸� SendFlush�� �ؾ߸� ���� ������ �̷�����. */
	void SendHeader(int msg,int sub=0);
	void SendVer(int ver) { m_msgHead.ver = ver; }
	void SendUser(int user) { m_msgHead.user = user; }
	void SendSet(int val);
	void SendSet(unsigned int val);
	void SendSet(long val);
	void SendSet(float val);
	void SendSet(double val);
	void SendSet(myint64 val);
	void SendSet(char* val,int size);
	void SendSetStr(char *val, int size);
	/** Zenius���� ����ϴ� Protocol���� ����ϴ� �Լ��μ� send-buffer�� ������ ���� �����Ѵ�.
	 return value > 0, if success */
	int  SendFlush(int opt=0);

	/** Zenius���� ����ϴ� Protocol���� ����ϴ� �Լ��μ� recieve buffer�� ���� ������ �����Ѵ�.
	 return value > 0, if success */
	int  RecvHeader(int& msg,int opt=0);
	/** Zenius���� ����ϴ� Protocol���� ����ϴ� �Լ��μ� recieve buffer���� �о�´�. 
	 return value > 0, if success */	
	int  RecvMsg() { return m_msgHead.msg; }
	int  RecvSub() { return m_msgHead.sub; }
	int  RecvLen() { return m_msgHead.len; }
	int  RecvVer() { return m_msgHead.ver; }
	int  RecvUser() { return m_msgHead.user; }
	void RecvLen(int& val,int size);
	void RecvGet(int& val);
	void RecvGet(unsigned int& val);
	void RecvGet(long& val);
	void RecvGet(float& val);
	void RecvGet(double& val);
	void RecvGet(myint64& val);
	void RecvGet(char* val,int size);
	/** Zenius���� ����ϴ� Protocol���� ����ϴ� �Լ��μ� recieve buffer���� �о�´�. 
	 SendSetStr(char* val, int size)�� ����Ұ�� �ݵ�� ���Լ��� ����ؾ� �Ѵ�.
	 return value > 0, if success */	
	void RecvGetStr(char *val, int size);

	int	 GetSockName(char* addr,int size,int& port,int fd=0);
	int  GetPeerName(char* addr,int size,int& port,int fd=0);
	unsigned int GetPeerAddr(int fd=0);
#ifndef _USE_IPV4ONLY
	/** v4 �ϰ�� ip���� �����ϰ� �ǰ�, v6�ϰ�� �����ϸ� 1, �ƴϸ� 0�� �����ϸ�, �������
	 ���ڷ� �����ϰ� �ȴ�. */
	unsigned int GetPeerAddr(struct in6_addr *sin6_addr, int fd=0);
#endif

	// close
    virtual void Close(int fd=0);

	// buffer
	char*       GetDataBuf(int size);
	char*       GetSendBuf(int size);
	char*       GetRecvBuf(int size);

	//! socket file descript���� �����ϴ� �Լ�
	void SetSocket(int fd);
	//! �����ϰ����ϴ� Host������ �����ϴ� �Լ�
	void SetHost(char* pHost,int nPort);

	//! m_nSockFd�� 0�̻��̸� 1�� �����ϰ� �ƴϸ� 0�� �����Ѵ�.
    int		IsConnected() { return (m_nSockFd > 0 ? 1 : 0); }
	//! ȣ��Ʈ������ �����Ѵ�.
	char*	GetHost() { return m_szHost; }
	//! ��Ʈ������ �����Ѵ�.
	int		GetPort() { return m_nPort; }

	// added by kskim (req. hjson) 2006.3.15, modifyed 2006.03.26
	//! ���н� �� ���������� ���Ǵ°����μ� getrlimit �Լ��� ����� ����� �� �ִ�.
	void 	GetFdLimit(int &hardlimit, int &curlimit);
	/** ���н� �� ���������� ���Ǵ°����μ� setrlimit �Լ��� ����� ����� �� �ִ�.
	���������� getrlimit �Լ��� ���� ���� ������ ��� 1�� �����ϰ� ������ ��� -1�� �����Ѵ�. */
	int 	SetFdLimit(int newlimit); // if success retrun 0 else return -1
	/** listen�Ҷ� backlog ���� ������ �� �ִ�. */
	void 	SetBacklog(int nBacklog=5);

	//! ���Ͽ� broadcast������ �Ѵ�.
    int SetBroadcast();
};

float htonf(float val);
float ntohf(float val);
double htond(double val);
double ntohd(double val);
myint64  htonint64(myint64 val);
myint64  ntohint64(myint64 val);
umyint64 uiuitouint64(unsigned int low,unsigned int high);
myint64 uiuitoint64(unsigned int low,unsigned int high);
void int64touiui(myint64 val,unsigned int& low,unsigned int& high);

// inet_ntop�� ���� ������ (windows ���� ���������� inet_ntop �� ���ѳ����� 
// �����Ƿ� �������� (windows 2008, vista �̻���� ������)
// ���н�/�������� ���ؼ��� �������� ������� �̺κп��� ó���� �� ����
const char *myinet_ntop(int af, const void *src, char *dst, mysocklen_t cnt);
int myinet_pton(int af, const char *src, void *dst);


/////////////////////////////////////////////////////////////////
// utility functions; num's order is host-byte order (2005.03.17)

//! ���ڷ� �־��� ���ڿ� IP�� ���� ������ ������ ��ȯ�ϴ� �Լ�
unsigned int	IP2Num(char* ip);
//! ���ڷ� �־��� ���� ������ IP�� ���ڿ��� ��ȯ�ϴ� �Լ�
void			Num2IP(unsigned int num,char* ip,int size);
//! ���ڷ� �־��� ���� ������ IP�� ���ڿ��� ��ȯ�ϴ� �Լ�
void			Num2IP(int num,char* ip,int size);
//! ���ڷ� �־��� ���� ������ IP�� ���ڿ��� ��ȯ�ϴ� �Լ�
void			Num2IP(long num,char* ip,int size);
//! ���ڷ� �־��� ���� ������ IP�� ���ڿ��� ��ȯ�ϴ� �Լ�
void			Num2IP(unsigned long num,char* ip,int size);
//! ���ڷ� �־��� ���� ������ IP�� in_addr�� ��ȯ�ϴ� �Լ�
struct in_addr	Num2InAddr(unsigned int num);
//! ���ڷ� �־��� in_addr IP�� ���� ���·� ��ȯ�ϴ� �Լ�
unsigned int	InAddr2Num(struct in_addr& in);
//! ���ڷ� �־��� sockaddr_in�� ���� ���·� ��ȯ�ϴ� �Լ�
unsigned int	SockAddrIn2Num(struct sockaddr_in& in);
/*! ���ڷ� �־��� sockaddr�� ���� ���·� ��ȯ�ϴ� �Լ� */
unsigned int	SockAddr2Num(struct sockaddr& in);
/*! ���ڷ� �־��� IP�� ������ �����ؼ� inet family���� �����Ѵ�. 
  AF_INET, AF_INET6 */
int				GetIPVersion(char *ip);
#endif
