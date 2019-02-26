/*
   Copyright (c) 2002-2010 BrainzSquare, Inc.
   icmpmgr6.h - CIcmpMgr6
 */

#ifndef __ICMPMGR6_H__
#define __ICMPMGR6_H__

#ifdef _WIN32
#elif _UNIXWARE
#include <netinet/in.h>
#else

#if  defined(_FREEBSD) || defined(_HPUX)
#include <netinet/in_systm.h>
#endif

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
//#include <netinet/ip6_icmp.h>
#include <netinet/icmp6.h>
#endif

#include "icmpmgr.h"

struct Icmp6Item {
	struct in6_addr	ip;	
	long			ts;
	unsigned short  seq; 
};

struct ReqAddr6 {
	struct in6_addr ip;
	unsigned int	scope_id;
};

//! PING üũ�� ���� Ŭ�����μ� ���� IP�뿪�� PING�� broadcasting�Ͽ� ������� �����Ѵ�.
class CIcmpMgr6 {
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

	// IPv6 ICMP Packet logging
	// 0�̸� ECHO_REPLY�� �ް� �ٸ��� ���͸�
	int m_nVerbose;
	// Scope ID
	int m_nScopeId;
	// ICMP protocol type
	int m_nIcmpProto;

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

	// add by lordang 2010.09.13
	//! ������ �� IPv6 ���� ���°�
    Icmp6Item	m_sRespIp6[ICMPMGR_MAX_NUMIP];
	//! IPv6 PING üũ�� ���� �Է��� IP��� (network byte order)
	struct ReqAddr6	m_sReqIP6[ICMPMGR_MAX_NUMIP];
	//

	// PING Packet�� 3���� ������ ���� 2008.07.10 by hjson
	int 		m_nReqTry[ICMPMGR_MAX_NUMIP];
	// ping request ���� �ð� by lordang 2009.10.28
	unsigned int	m_uReqSendTm[ICMPMGR_MAX_NUMIP];
	// user data 2008.07.10 by hjson
	void* 		m_nReqId[ICMPMGR_MAX_NUMIP];

public:
    CIcmpMgr6();
    ~CIcmpMgr6();
    
	//! PING üũ�� ���� Socket ���� �Լ� (IPPROTO_ICMPV6 or IPPROTO_ICMP)
    int  Open(int icmpproto=IPPROTO_ICMP); // modi by lordang
	int  Open6() { return Open(IPPROTO_ICMPV6); }

	// Echo Reponse ���� ICMP message ���� �� ��� ����(1: ����, 0: ���͸�)
	void SetVerbose(int verbose) { m_nVerbose = verbose; }
	// Interface Scope ID ����
	void SetScopeId(int scopeid) { m_nScopeId = scopeid; }
	//! Socket TTL������ �Է��ϴ� �Լ� (���� �ƹ��� ��� ����)
    void SetTTL(int ttl);
	//! �Է��� IP�뿪���� ���� ������ �ִ� �����ǵ��� ã�Ƴ���.
    int  Find(char* pFrom,char* pTo,int nTimeout,int nRetry=3);	// seconds
    int  Trace(char* pAddr,int nTimeout=5,int nHop=30,int nProbe=3);
    void Close();

	// IPv6�� �Ѱ��� �Է��ϴ� �Լ�
	int Add(char* pHost,void* pId=NULL); // ���� �ּ� ���� �Է�
	int Add(struct sockaddr_in6 sa6,void* pId=NULL); // ������ �ּ� �Է�
	int Add(unsigned int uIP,void* pId=NULL); 	// 2008.07.10 hjson

	//! IP�� �Է��� �Ǿ� �ִٸ� 1�� �����ϰ� �ԷµǾ� ���� �ʴٸ� -1�� �Է��Ѵ�.
	int Hash(unsigned int addr,unsigned short seqno);
	int Hash(struct in6_addr addr,unsigned short seqno);

	//! �Է��� IP�� ���ؼ� Ping üũ�� �Ѵ�.
    long Ping(char* pAddr,int nTimeout, int nRetry=3);
	//! ����� IP�鿡 ���ؼ� Ping üũ�� �Ѵ�.
	int Ping(int nTimeout,int nRetry=5);
	
	//! ��� ������� �Է��� IP����� �����.
	int Clear();
	//! IDENTITY Key�� �����ϱ����� �Լ� // 2010.4.6 kskim
	void SetIdent(unsigned short ident) { m_nIdent = ident; }

	// added by lordang, kskim
    int  DecodeResp(char* buf, int bytes,
		struct sockaddr* from,long& ts,unsigned short& seqno,int mytype);
    int  DecodeResp(char* buf, int bytes,
		struct sockaddr_in* from,long& ts,unsigned short& seqno,int mytype);
    int  DecodeResp(char* buf, int bytes,
		struct sockaddr_in6* from,long& ts,unsigned short& seqno,int mytype);

	// modified by kskim. 2010.12.8
    int  SendTo(unsigned short seqno,char* data,int datasize,char *pszAddr);
    int  SendTo(unsigned short seqno,char* data,int datasize,sockaddr_in& dest);
    int  SendTo(unsigned short seqno,char* data,int datasize,sockaddr_in6& dest);
    int  SendTo(unsigned short seqno,char* data,int datasize,sockaddr* dest);

	// modified by lordang
    int  RecvFrom(char* buf,int len, sockaddr_in& from,mysocklen_t& fromlen);
    int  RecvFrom(char* buf,int len, sockaddr_in6& from,mysocklen_t& fromlen);
    int  RecvFrom(char* buf,int len, sockaddr* from,mysocklen_t& fromlen);

	// added by lordang, kskim
	int	 RecvMsg(char *recvbuf, sockaddr *from, mysocklen_t fromlen);

protected:
    void FillData(char* data, int datasize, int icmpproto=IPPROTO_ICMP);
    void FillV4Data(char* data, int datasize); // add by lordang
    void FillV6Data(char* data, int datasize); // add by lordang
	//
    int  FindV4(char* pFrom,char* pTo,int nTimeout,int nRetry=3);	// seconds
    int  FindV6(char* pFrom,char* pTo,int nTimeout,int nRetry=3);	// seconds
	//
	int PingV4(int nTimeout,int nRetry=5);
	int PingV6(int nTimeout,int nRetry=5);

	//! ICMPv6 Ping �� Trace ������ �´� ���͸� ����
	void SetIcmp6Filter(int mytype);
	//! address compare
	int CompAddr(struct sockaddr* sa1, struct sockaddr* sa2, mysocklen_t salen);

    unsigned short Checksum(unsigned short* buffer,int size);
	// ipv6 checksum (winsock�� ����) added by lordang
	unsigned short ChecksumIcmp6(int s, char *icmppacket, int icmplen, struct sockaddr_in6& dest);

};

#if defined( _WIN32) || defined(_UNIXWARE)
struct icmp6_hdr
{
	unsigned char     icmp6_type;   /* type field */
	unsigned char     icmp6_code;   /* code field */
	unsigned short    icmp6_cksum;  /* checksum field */
	union
	{
		unsigned int  icmp6_un_data32[1]; /* type-specific field */
		unsigned short  icmp6_un_data16[2]; /* type-specific field */
		unsigned char   icmp6_un_data8[4];  /* type-specific field */
	} icmp6_dataun;
};

#define icmp6_data32    icmp6_dataun.icmp6_un_data32
#define icmp6_data16    icmp6_dataun.icmp6_un_data16
#define icmp6_data8     icmp6_dataun.icmp6_un_data8
#define icmp6_pptr      icmp6_data32[0]  /* parameter prob */
#define icmp6_mtu       icmp6_data32[0]  /* packet too big */
#define icmp6_id        icmp6_data16[0]  /* echo request/reply */
#define icmp6_seq       icmp6_data16[1]  /* echo request/reply */
#define icmp6_maxdelay  icmp6_data16[0]  /* mcast group membership */

#define ICMP6_DST_UNREACH             1
#define ICMP6_PACKET_TOO_BIG          2
#define ICMP6_TIME_EXCEEDED           3
#define ICMP6_PARAM_PROB              4

#define ICMP6_INFOMSG_MASK  0x80    /* all informational messages */

#define ICMP6_ECHO_REQUEST          128
#define ICMP6_ECHO_REPLY            129
#define ICMP6_MEMBERSHIP_QUERY      130
#define ICMP6_MEMBERSHIP_REPORT     131
#define ICMP6_MEMBERSHIP_REDUCTION  132

#define ICMP6_DST_UNREACH_NOROUTE     0 /* no route to destination */
#define ICMP6_DST_UNREACH_ADMIN       1 /* communication with destination */
                                        /* administratively prohibited */
#define ICMP6_DST_UNREACH_NOTNEIGHBOR 2 /* not a neighbor */
#define ICMP6_DST_UNREACH_ADDR        3 /* address unreachable */
#define ICMP6_DST_UNREACH_NOPORT      4 /* bad port */

#define ICMP6_TIME_EXCEED_TRANSIT     0 /* Hop Limit == 0 in transit */
#define ICMP6_TIME_EXCEED_REASSEMBLY  1 /* Reassembly time out */

#define ICMP6_PARAMPROB_HEADER        0 /* erroneous header field */
#define ICMP6_PARAMPROB_NEXTHEADER    1 /* unrecognized Next Header */
#define ICMP6_PARAMPROB_OPTION        2 /* unrecognized IPv6 option */

#define IN6_IS_ADDR_V4MAPPED(a) \
	((((const unsigned int *) (a))[0] == 0)				      \
	 && (((const unsigned int *) (a))[1] == 0)			      \
	 && (((const unsigned int *) (a))[2] == htonl (0xffff)))

#endif // _WIN32

#endif
