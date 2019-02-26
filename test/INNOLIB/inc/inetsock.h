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

//! 소켓사용을 위한 최상위 클래스
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

	/** 입력값을 전송하는 함수  return value > 0, if success */
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
	/** Zenius에서 사용하는 Protocol에서 사용하는 함수로서 send-buffer에 헤더정보를 
	 설정하는 함수 이며 SendFlush를 해야만 실제 전송이 이뤄진다. */
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
	/** Zenius에서 사용하는 Protocol에서 사용하는 함수로서 send-buffer의 내용을 실제 전송한다.
	 return value > 0, if success */
	int  SendFlush(int opt=0);

	/** Zenius에서 사용하는 Protocol에서 사용하는 함수로서 recieve buffer에 전송 내용을 수신한다.
	 return value > 0, if success */
	int  RecvHeader(int& msg,int opt=0);
	/** Zenius에서 사용하는 Protocol에서 사용하는 함수로서 recieve buffer에서 읽어온다. 
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
	/** Zenius에서 사용하는 Protocol에서 사용하는 함수로서 recieve buffer에서 읽어온다. 
	 SendSetStr(char* val, int size)를 사용할경우 반드시 이함수를 사용해야 한다.
	 return value > 0, if success */	
	void RecvGetStr(char *val, int size);

	int	 GetSockName(char* addr,int size,int& port,int fd=0);
	int  GetPeerName(char* addr,int size,int& port,int fd=0);
	unsigned int GetPeerAddr(int fd=0);
#ifndef _USE_IPV4ONLY
	/** v4 일경우 ip값을 리턴하게 되고, v6일경우 성공하면 1, 아니면 0을 리턴하며, 결과값은
	 인자로 전달하게 된다. */
	unsigned int GetPeerAddr(struct in6_addr *sin6_addr, int fd=0);
#endif

	// close
    virtual void Close(int fd=0);

	// buffer
	char*       GetDataBuf(int size);
	char*       GetSendBuf(int size);
	char*       GetRecvBuf(int size);

	//! socket file descript값을 지정하는 함수
	void SetSocket(int fd);
	//! 접속하고자하는 Host정보를 지정하는 함수
	void SetHost(char* pHost,int nPort);

	//! m_nSockFd가 0이상이면 1을 리턴하고 아니면 0을 리턴한다.
    int		IsConnected() { return (m_nSockFd > 0 ? 1 : 0); }
	//! 호스트정보를 리턴한다.
	char*	GetHost() { return m_szHost; }
	//! 포트정보를 리턴한다.
	int		GetPort() { return m_nPort; }

	// added by kskim (req. hjson) 2006.3.15, modifyed 2006.03.26
	//! 유닉스 및 리눅스에서 사용되는것으로서 getrlimit 함수를 대신해 사용할 수 있다.
	void 	GetFdLimit(int &hardlimit, int &curlimit);
	/** 유닉스 및 리눅스에서 사용되는것으로서 setrlimit 함수를 대신해 사용할 수 있다.
	내부적으로 getrlimit 함수를 통해 설정 가능할 경우 1을 리턴하고 실패할 경우 -1을 리턴한다. */
	int 	SetFdLimit(int newlimit); // if success retrun 0 else return -1
	/** listen할때 backlog 수를 조절할 수 있다. */
	void 	SetBacklog(int nBacklog=5);

	//! 소켓에 broadcast설정을 한다.
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

// inet_ntop에 대한 재정의 (windows 낮은 버전에서는 inet_ntop 에 대한내용이 
// 없으므로 재정의함 (windows 2008, vista 이상부터 지원됨)
// 유닉스/리눅스에 대해서도 지원하지 않을경우 이부분에서 처리될 수 있음
const char *myinet_ntop(int af, const void *src, char *dst, mysocklen_t cnt);
int myinet_pton(int af, const char *src, void *dst);


/////////////////////////////////////////////////////////////////
// utility functions; num's order is host-byte order (2005.03.17)

//! 인자로 주어진 문자열 IP를 숫자 형태의 값으로 변환하는 함수
unsigned int	IP2Num(char* ip);
//! 인자로 주어진 숫자 형태의 IP를 문자열로 변환하는 함수
void			Num2IP(unsigned int num,char* ip,int size);
//! 인자로 주어진 숫자 형태의 IP를 문자열로 변환하는 함수
void			Num2IP(int num,char* ip,int size);
//! 인자로 주어진 숫자 형태의 IP를 문자열로 변환하는 함수
void			Num2IP(long num,char* ip,int size);
//! 인자로 주어진 숫자 형태의 IP를 문자열로 변환하는 함수
void			Num2IP(unsigned long num,char* ip,int size);
//! 인자로 주어진 숫자 형태의 IP를 in_addr로 변환하는 함수
struct in_addr	Num2InAddr(unsigned int num);
//! 인자로 주어진 in_addr IP를 숫자 형태로 변환하는 함수
unsigned int	InAddr2Num(struct in_addr& in);
//! 인자로 주어진 sockaddr_in를 숫자 형태로 변환하는 함수
unsigned int	SockAddrIn2Num(struct sockaddr_in& in);
/*! 인자로 주어진 sockaddr를 숫자 형태로 변환하는 함수 */
unsigned int	SockAddr2Num(struct sockaddr& in);
/*! 인자로 주어진 IP의 정보를 수집해서 inet family값을 리턴한다. 
  AF_INET, AF_INET6 */
int				GetIPVersion(char *ip);
#endif
