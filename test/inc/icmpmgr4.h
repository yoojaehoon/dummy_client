/*
   Copyright (c) 2002-2010 BrainzSquare, Inc.
   icmpmgr4.h - CIcmpMgr4
 */

#ifndef __ICMPMGR4_H__
#define __ICMPMGR4_H__

#include "icmpmgr.h"

//! PING 체크를 위한 클래스로서 일정 IP대역에 PING를 broadcasting하여 결과값을 저장한다.
class CIcmpMgr4 {
public:
    int				m_nSockRaw;
    int				m_nReqNum;
    int				m_nStart;
    int				m_nTimeout;

protected:
	unsigned int	m_nFrom;
	unsigned int	m_nTo;

#ifdef _WIN32
	DWORD   m_tv1, m_tv2;
#else
	struct timeval  m_tv1, m_tv2;
#endif

	// added by kskim. 2010.4.6
	unsigned short	m_nIdent;
public:
	//! 응답이 온 IP들의 갯수
    int			m_nRespNum;
	//! 응답이 온 IP들의 상태값으로서 성공이면 1의 값을 같게 된다.
    int			m_nRespOk[ICMPMGR_MAX_NUMIP];
	//! 응답이 온 IP들의 상태값
    IcmpItem	m_sRespIp[ICMPMGR_MAX_NUMIP];

	// added by kskim. 20070409
	//! PING 체크를 위해 입력한 IP목록 (host byte order)
	unsigned long   m_nReqIP[ICMPMGR_MAX_NUMIP];
	// PING Packet을 3번만 보내기 위해 2008.07.10 by hjson
	int 		m_nReqTry[ICMPMGR_MAX_NUMIP];
	// ping request 보낸 시간 by lordang 2009.10.28
	unsigned int	m_uReqSendTm[ICMPMGR_MAX_NUMIP];
	// user data 2008.07.10 by hjson
	void* 		m_nReqId[ICMPMGR_MAX_NUMIP];

public:
    CIcmpMgr4();
    ~CIcmpMgr4();
    
	//! PING 체크를 위한 Socket 열기 함수
    int  Open();
	//! Socket TTL정보를 입력하는 함수 (현재 아무런 기능 없음)
    void SetTTL(int ttl);
	//! 입력한 IP대역에서 현재 응답이 있는 아이피들을 찾아낸다.
    int  Find(char* pFrom,char* pTo,int nTimeout,int nRetry=3);	// seconds
	//! 입력한 IP에 대해서 Ping 체크를 한다.
    long Ping(char* pAddr,int nTimeout, int nRetry=3);
    int  Trace(char* pAddr,int nTimeout=5,int nHop=30,int nProbe=3);
    void Close();
	//{{ added by kskim. 20070409
	//! IP를 한개씩 입력하는 함수
	int Add(unsigned int uIP,void* pId=NULL); 	// 2008.07.10 hjson
	int Add(char* pHost,void* pId=NULL); // 2010.12.8 kskim
	//! IP가 입력이 되어 있다면 1을 리턴하고 입력되어 있지 않다면 -1을 입력한다.
	int Hash(unsigned int addr,unsigned short seqno);
	//! 저장된 IP들에 대해서 Ping 체크를 한다.
	int Ping(int nTimeout,int nRetry=5);
	//}} added by kskim. 20070409
	//! 모든 결과값과 입력한 IP목록을 지운다.
	int Clear();
	//! IDENTITY Key를 변경하기위한 함수 // 2010.4.6 kskim
	void SetIdent(unsigned short ident) { m_nIdent = ident; }

    void FillData(char* data, int datasize);
    unsigned short Checksum(unsigned short* buffer,int size);
    int  DecodeResp(char* buf, int bytes,
		struct sockaddr_in& from,long& ts,unsigned short& seqno,int mytype);
    int  SendTo(unsigned short seqno,char* data,int datasize,
		struct sockaddr_in& dest);
    int  RecvFrom(char* buf,int len,
	    struct sockaddr_in& from,mysocklen_t& fromlen);
};

#ifdef _WIN32
typedef struct tagIP_OPTION_INFORMATION_MY 
{
	unsigned char      Ttl;              // Time To Live
	unsigned char      Tos;              // Type Of Service
	unsigned char      Flags;            // IP header flags
	unsigned char      OptionsSize;      // Size in bytes of options data
	unsigned char FAR *OptionsData;      // Pointer to options data
} IP_OPTION_INFORMATION_MY;

typedef struct tagICMP_ECHO_REPLY_MY 
{
	unsigned long		  Address;       // Replying address
	unsigned long         Status;        // Reply IP_STATUS
	unsigned long         RoundTripTime; // RTT in milliseconds
	unsigned short        DataSize;      // Reply data size in bytes
	unsigned short        Reserved;      // Reserved for system use
	void FAR              *Data;         // Pointer to the reply data
	IP_OPTION_INFORMATION_MY Options;       // Reply options
} ICMP_ECHO_REPLY_MY;
#endif

#endif
