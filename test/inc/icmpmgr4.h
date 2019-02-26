/*
   Copyright (c) 2002-2010 BrainzSquare, Inc.
   icmpmgr4.h - CIcmpMgr4
 */

#ifndef __ICMPMGR4_H__
#define __ICMPMGR4_H__

#include "icmpmgr.h"

//! PING üũ�� ���� Ŭ�����μ� ���� IP�뿪�� PING�� broadcasting�Ͽ� ������� �����Ѵ�.
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
	//! ������ �� IP���� ����
    int			m_nRespNum;
	//! ������ �� IP���� ���°����μ� �����̸� 1�� ���� ���� �ȴ�.
    int			m_nRespOk[ICMPMGR_MAX_NUMIP];
	//! ������ �� IP���� ���°�
    IcmpItem	m_sRespIp[ICMPMGR_MAX_NUMIP];

	// added by kskim. 20070409
	//! PING üũ�� ���� �Է��� IP��� (host byte order)
	unsigned long   m_nReqIP[ICMPMGR_MAX_NUMIP];
	// PING Packet�� 3���� ������ ���� 2008.07.10 by hjson
	int 		m_nReqTry[ICMPMGR_MAX_NUMIP];
	// ping request ���� �ð� by lordang 2009.10.28
	unsigned int	m_uReqSendTm[ICMPMGR_MAX_NUMIP];
	// user data 2008.07.10 by hjson
	void* 		m_nReqId[ICMPMGR_MAX_NUMIP];

public:
    CIcmpMgr4();
    ~CIcmpMgr4();
    
	//! PING üũ�� ���� Socket ���� �Լ�
    int  Open();
	//! Socket TTL������ �Է��ϴ� �Լ� (���� �ƹ��� ��� ����)
    void SetTTL(int ttl);
	//! �Է��� IP�뿪���� ���� ������ �ִ� �����ǵ��� ã�Ƴ���.
    int  Find(char* pFrom,char* pTo,int nTimeout,int nRetry=3);	// seconds
	//! �Է��� IP�� ���ؼ� Ping üũ�� �Ѵ�.
    long Ping(char* pAddr,int nTimeout, int nRetry=3);
    int  Trace(char* pAddr,int nTimeout=5,int nHop=30,int nProbe=3);
    void Close();
	//{{ added by kskim. 20070409
	//! IP�� �Ѱ��� �Է��ϴ� �Լ�
	int Add(unsigned int uIP,void* pId=NULL); 	// 2008.07.10 hjson
	int Add(char* pHost,void* pId=NULL); // 2010.12.8 kskim
	//! IP�� �Է��� �Ǿ� �ִٸ� 1�� �����ϰ� �ԷµǾ� ���� �ʴٸ� -1�� �Է��Ѵ�.
	int Hash(unsigned int addr,unsigned short seqno);
	//! ����� IP�鿡 ���ؼ� Ping üũ�� �Ѵ�.
	int Ping(int nTimeout,int nRetry=5);
	//}} added by kskim. 20070409
	//! ��� ������� �Է��� IP����� �����.
	int Clear();
	//! IDENTITY Key�� �����ϱ����� �Լ� // 2010.4.6 kskim
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
