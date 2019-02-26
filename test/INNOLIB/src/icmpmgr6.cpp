/*
   Copyright (c) 2002-2010 BrainzSquare, Inc.
   icmpmgr.cpp - icmpmgr ping
   
   2005.03.04. modified Find, Trace
   FIX ME: trace may be incorrect at some circumstance

	2010.08.30 modified Open() by lordang
	- Send Buffer와 Recv Buffer가 작으면 늘려주도록 수정 (1024*1024)

	2010.11.02 modified by lordang
	- ICMPv6 지원 함수 수정 및 추가

	2010.11.03 modified by kskim
	- GetAddrInfo 함수 추가, getaddrinfo 함수 사용을 위한것

	2010.11.22 modified by kskim
	- GetAddrInfo 함수 사용할때 리턴값 체크하도록 수정
	- SendTo(..., char *pszAddr) 함수 추가 (자동으로 IPv4, IPv6판단하도록 수정)

	2010.11.23 modified by kskim
	- RecvMsg(...) 함수 변경, 함수 인자 변경
	- DecodeResp(...) 함수 변경, 함수 인자 변경

	2010.11.26 modified by kskim
	- Trace(...) 함수 오류수정
	 - ts < 0 작을 경우 바로 리턴하게 수정
	 - IPv6일 경우 ts값을 안넘기는 문제 수정
 */

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#ifdef _AIX
#include <strings.h>
#endif
#include <sys/types.h>

#if defined(_UNIXWARE) || defined(_FREEBSD)
#include <sys/uio.h>
#endif

// std
#include <string>

#include "icmpmgr6.h"
#include "tcpsock.h"

// on aix52
#if !defined(ICMP6_ECHO_REQUEST) && defined(ICMPV6_ECHOREQUEST)
#define ICMP6_ECHO_REQUEST (ICMPV6_ECHOREQUEST)
#endif
#if !defined(ICMP6_TIME_EXCEEDED) && defined(ICMPV6_TIME_EXCEEDED)
#define ICMP6_TIME_EXCEEDED (ICMPV6_TIME_EXCEEDED)
#endif
#if !defined(ICMP6_DST_UNREACH) && defined(ICMPV6_DEST_UNREACH)
#define ICMP6_DST_UNREACH (ICMPV6_DEST_UNREACH)
#endif
#if !defined(ICMP6_ECHO_REPLY) && defined(ICMPV6_ECHOREPLY)
#define ICMP6_ECHO_REPLY (ICMPV6_ECHOREPLY)
#endif
#if !defined(ICMP6_HOPLIMIT) && defined(ICMPV6_HOPLIMIT)
#define ICMP6_HOPLIMIT (ICMPV6_HOPLIMIT)
#endif
#if !defined(ICMP6_DST_UNREACH_NOPORT) && defined(ICMPV6_DEST_UNREACH_NOPORT)
#define ICMP6_DST_UNREACH_NOPORT (ICMPV6_DEST_UNREACH_NOPORT)
#endif

static addrinfo* GetAddrInfo(char* pHost)
{
	int en = 0;
	struct addrinfo	hints, *res = NULL;
	memset(&hints, 0x00, sizeof(struct addrinfo));
	hints.ai_flags = AI_CANONNAME;	/* always return canonical name */
	hints.ai_family = AF_UNSPEC;		/* 0, AF_INET, AF_INET6, etc. */
	hints.ai_socktype = 0;	/* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

	if ( (en = getaddrinfo(pHost, NULL, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error for %s: %s\n",
				pHost, gai_strerror(en));
		return NULL;
	}
	//
	return res;
}

#ifdef _HJSON
static bool CheckUnixScopeId(const std::string& sDest, std::string& sHost, std::string& sIf)
{
	std::string::size_type nFirst = 0;
	std::string::size_type nLast  = 0;
	std::string::size_type nOffset= 0;
//	std::string::size_type nFile  = 0;

	nFirst  = sDest.find_last_of("%");
	if(nFirst == std::string::npos) return false;
	if(nFirst == 0) return false;
	
	// 호스트명 뽑기
	sHost = sDest.substr(0, nFirst);
	nLast   = sDest.size();
	// 인터페이스명 뽑기
	nFirst  = nFirst + 1; // % 제외
	nOffset = nLast - nFirst;
	sIf = sDest.substr(nFirst, nOffset);
	
	return true;
}
#endif

CIcmpMgr6::CIcmpMgr6()
{
    m_nSockRaw = -1;
    m_nRespNum = 0;
    m_nTimeout = 5;
    m_nReqNum = 0; // -1 => 0 by kskim, shnoh 2007.05.31
    m_nStart = 0;
	m_nVerbose = 0;
	m_nScopeId = 0;
	m_nIcmpProto = IPPROTO_ICMP;

	memset(m_nRespOk,0,sizeof(int)*ICMPMGR_MAX_NUMIP);
	memset(m_sRespIp,0,sizeof(IcmpItem)*ICMPMGR_MAX_NUMIP);
	memset(m_nReqIP,0,sizeof(unsigned long)*ICMPMGR_MAX_NUMIP);
	memset(m_nReqId,0,sizeof(void*)*ICMPMGR_MAX_NUMIP);
	memset(m_nReqTry,0,sizeof(int)*ICMPMGR_MAX_NUMIP);
	memset(m_uReqSendTm,0,sizeof(unsigned int)*ICMPMGR_MAX_NUMIP);
	// added by kskim. 2010.4.6
	m_nIdent = (unsigned short) ICMPMGR_IDENT;

	memset(m_sRespIp6,0,sizeof(Icmp6Item)*ICMPMGR_MAX_NUMIP);
	memset(m_sReqIP6,0,sizeof(ReqAddr6)*ICMPMGR_MAX_NUMIP);
}

CIcmpMgr6::~CIcmpMgr6()
{
    Close();
}

int CIcmpMgr6::Ping(int nTimeout,int nRetry)
{
	if(m_nIcmpProto == IPPROTO_ICMP) {
		m_nRespNum = PingV4(nTimeout, nRetry);
	} else if(m_nIcmpProto == IPPROTO_ICMPV6) {
		m_nRespNum = PingV6(nTimeout, nRetry);
	}

	return m_nRespNum;
}

int CIcmpMgr6::PingV4(int nTimeout,int nRetry)
{
	if(m_nReqNum ==0) return 0;
	if(nTimeout<=0) nTimeout = 5;

	int i;
	m_nRespNum = 0;
	for(i=0;i<m_nReqNum;i++) {
		m_uReqSendTm[i] = 0;
		m_nReqTry[i] = 0;
		m_nRespOk[i] = 0;
	}
	m_nTimeout = nTimeout;

	unsigned short seqno = 0, retno=0;
	char data[ICMPMGR_MAX_PACKET], recvbuf[ICMPMGR_MAX_PACKET];
	int datasize = ICMPMGR_DEF_PACKET_SIZE + sizeof(IcmpHeader);
	struct timeval wait;
	struct sockaddr_in from, dest;
	mysocklen_t fromlen = sizeof(from);
	fd_set rset, wset;
	int selfd, nread, nret, indx, next = 0;
	long ts;
	time_t tNow;

	// added select for wset 2008.07.10 by hjson
    m_nStart = time(NULL);
	while(m_nRespNum < m_nReqNum) {

		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(m_nSockRaw,&rset);
		FD_SET(m_nSockRaw,&wset);

		wait.tv_sec = 1; wait.tv_usec = 0;
		selfd = select(m_nSockRaw+1,&rset,&wset,NULL,&wait);

		tNow = time(NULL);

		if(selfd>0 && FD_ISSET(m_nSockRaw,&rset)) {
	       	nread = RecvFrom(recvbuf,ICMPMGR_MAX_PACKET,from,fromlen);
			if(nread > 0) {
				retno = seqno;
				nret = DecodeResp(recvbuf,nread,&from,ts,
					retno,ICMPMGR_MYTYPE_FIND);
				if(ts>=0 && nret==ICMPMGR_STATUS_OK) {
					indx = Hash(ntohl(from.sin_addr.s_addr),retno);
					if(indx>=0 && m_nRespOk[indx]==0) {
						m_nRespOk[indx] = 1;
						m_sRespIp[indx].ip = from.sin_addr;
						m_sRespIp[indx].ts = ts;
						m_nRespNum++;
					}
				}
			}
		} else if(selfd>0 && FD_ISSET(m_nSockRaw,&wset)) {
			if(!m_nRespOk[next] && m_nReqTry[next]<nRetry && tNow-m_uReqSendTm[next] >= 1) {
				m_nReqTry[next]++;
	    		memset(&dest,0,sizeof(dest));
	    		dest.sin_addr.s_addr = htonl(m_nReqIP[next]);
	    		dest.sin_family = AF_INET;
	    		memset(data,0,ICMPMGR_MAX_PACKET);
	    		seqno++;
	    		SendTo(seqno,data,datasize,dest);
				m_sRespIp[next].ip = dest.sin_addr; // 2010.09.13 by lordang
				m_sRespIp[next].seq = seqno; // 2008.08.13 hjson
				m_uReqSendTm[next] = tNow;
			}
			next = (next+1)%m_nReqNum;
#ifdef _WIN32
			Sleep(1); 		// _POSCO 10 ms => 1ms
#else
			usleep(1000); 	// _POSCO 10 ms => 1ms
#endif
		}
		if((int)(time(NULL)-m_nStart) >= m_nTimeout) break;
	}
    
	return m_nRespNum;
}

int CIcmpMgr6::PingV6(int nTimeout,int nRetry)
{
	if(m_nReqNum ==0) return 0;
	if(nTimeout<=0) nTimeout = 5;

	int i;
	m_nRespNum = 0;
	for(i=0;i<m_nReqNum;i++) {
		m_uReqSendTm[i] = 0;
		m_nReqTry[i] = 0;
		m_nRespOk[i] = 0;
	}
	m_nTimeout = nTimeout;

	unsigned short seqno = 0, retno=0;
	char data[ICMPMGR_MAX_PACKET], recvbuf[ICMPMGR_MAX_PACKET];
	int datasize = ICMPMGR_DEF_PACKET_SIZE;

	struct timeval wait;
	fd_set rset, wset;
	int selfd, nread, nret, indx, next = 0;
	long ts;
	time_t tNow;

	struct sockaddr from;
    struct sockaddr_in6	dest;
    mysocklen_t fromlen = sizeof(from);

	datasize += sizeof(icmp6_hdr);  
	memset(data,0,ICMPMGR_MAX_PACKET);

	// Set Icmp6 Filtering
	SetIcmp6Filter(ICMPMGR_MYTYPE_PING);

	// added select for wset 2008.07.10 by hjson
    m_nStart = time(NULL);
	while(m_nRespNum < m_nReqNum) {

		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(m_nSockRaw,&rset);
		FD_SET(m_nSockRaw,&wset);

		wait.tv_sec = 1; wait.tv_usec = 0;
		selfd = select(m_nSockRaw+1,&rset,&wset,NULL,&wait);

		tNow = time(NULL);

		if(selfd>0 && FD_ISSET(m_nSockRaw,&rset)) {

			nread = RecvMsg(recvbuf, &from, fromlen);
			if(nread > 0) {
				retno = seqno;
		    	nret = DecodeResp(recvbuf,nread,&from,ts,retno,ICMPMGR_MYTYPE_FIND);
				if(ts>=0 && nret==ICMPMGR_STATUS_OK) {
					struct sockaddr_in6 *from_in6 = (sockaddr_in6 *) &from;
					indx = Hash(from_in6->sin6_addr,retno);
					if(indx>=0 && m_nRespOk[indx]==0) {
						m_nRespOk[indx] = 1;
						memcpy(&m_sRespIp6[indx].ip, &(from_in6->sin6_addr), 
								sizeof(m_sRespIp6[indx].ip));
						m_sRespIp6[indx].ts = ts;
						m_nRespNum++;
					}
				}
			}
		} else if(selfd>0 && FD_ISSET(m_nSockRaw,&wset)) {
			if(!m_nRespOk[next] && m_nReqTry[next]<nRetry && tNow-m_uReqSendTm[next] >= 1) {
				m_nReqTry[next]++;
	    		memset(&dest,0,sizeof(dest));
				memcpy(&dest.sin6_addr, &m_sReqIP6[next].ip, sizeof(dest.sin6_addr));
	    		dest.sin6_family = AF_INET6;
#if !defined(_UNIXWARE) && !defined(_AIX)
				if(m_sReqIP6[next].scope_id) dest.sin6_scope_id = m_sReqIP6[next].scope_id;
#endif
	    		memset(data,0,ICMPMGR_MAX_PACKET);
	    		seqno++;
	    		SendTo(seqno,data,datasize,dest);
				memcpy(&m_sRespIp6[next].ip, &dest.sin6_addr, sizeof(m_sRespIp6[next].ip));
				m_sRespIp6[next].seq = seqno; // 2008.08.13 hjson
				m_uReqSendTm[next] = tNow;
			}
			next = (next+1)%m_nReqNum;
#ifdef _WIN32
			Sleep(1); 		// _POSCO 10 ms => 1ms
#else
			usleep(1000); 	// _POSCO 10 ms => 1ms
#endif
		}

		if((int)(time(NULL)-m_nStart) >= m_nTimeout) break;
	}
    
	return m_nRespNum;
}

int CIcmpMgr6::Hash(unsigned int addr,unsigned short seqno)
{
	for(int i=0;i<m_nReqNum;i++) {
		if(m_nReqIP[i] == addr && m_sRespIp[i].seq == seqno) return i;
	}
	return -1;
}

int CIcmpMgr6::Hash(struct in6_addr addr,unsigned short seqno)
{
	for(int i=0;i<m_nReqNum;i++) {
		if(memcmp(&m_sReqIP6[i].ip, &addr, sizeof(m_sReqIP6[i].ip)) == 0 
				&& m_sRespIp6[i].seq == seqno) return i;
	}
	return -1;
}

int CIcmpMgr6::Add(unsigned int uIP,void* pId)
{
	int ret=0;
	if(m_nReqNum < ICMPMGR_MAX_NUMIP) {
		ret = 1;
		m_nReqIP[m_nReqNum] = uIP;
		m_nReqId[m_nReqNum] = pId; 		// 2008.07.10 by hjson
		m_nReqNum++;
	}
	return ret;
}

int CIcmpMgr6::Add(struct sockaddr_in6 sa6,void* pId)
{
	int ret=0;
	if(IN6_IS_ADDR_V4MAPPED(&sa6.sin6_addr)) {
		printf("cannot ping IPv4-mapped IPv6 address\n");
		return 0;
	}
	if(m_nReqNum < ICMPMGR_MAX_NUMIP) {
		ret = 1;
		memcpy(&m_sReqIP6[m_nReqNum].ip, &sa6.sin6_addr, sizeof(in6_addr));
#if !defined(_UNIXWARE) && !defined(_AIX)
		m_sReqIP6[m_nReqNum].scope_id = sa6.sin6_scope_id; // scope id
#endif
		m_nReqId[m_nReqNum] = pId; 		// 2008.07.10 by hjson
		m_nReqNum++;
	}
	return ret;
}

int CIcmpMgr6::Add(char* pHost,void* pId)
{
	int ret=0;

	struct addrinfo	*res;
	res = GetAddrInfo(pHost);
	if(res) {
		if(res->ai_family == AF_INET) {
			// host byte order
			ret = Add(ntohl(((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr), pId);
		} else if(res->ai_family == AF_INET6) {
			ret = Add(*(struct sockaddr_in6*)res->ai_addr, pId);
		}
	}
	freeaddrinfo(res);

	return ret;
}

// added by kskim. 20070425
int CIcmpMgr6::Clear()
{
	m_nReqNum = 0;
	return 0;
}

int CIcmpMgr6::Open(int icmpproto)
{
	int family = 0;
	if(icmpproto == IPPROTO_ICMP) {
		family = AF_INET;
	} else if(icmpproto == IPPROTO_ICMPV6) {
		family = AF_INET6;
	}

    m_nSockRaw = socket(family, SOCK_RAW, icmpproto);
    if (m_nSockRaw < 0) {
		printf("CIcmpMgr6::Open failed to create socket\n");
#ifdef _WIN32
		printf("CIcmpMgr6::Open : error(%d)\n", WSAGetLastError());
#else
		perror("CIcmpMgr6::Open ");
#endif
		return -1;
	}
	m_nIcmpProto = icmpproto;

	// _POSCO
	int ori, dst,  ret;
	mysocklen_t len;

	// set send buffer
	len = sizeof(ori);
	if(getsockopt(m_nSockRaw,SOL_SOCKET,SO_SNDBUF,(char*)&ori,(mysocklen_t *)&len) == 0) {
		if(ori <= 1024*1024) {
			dst = 1024*1024;
			len = sizeof(dst);
			ret = setsockopt(m_nSockRaw,SOL_SOCKET,SO_SNDBUF,(char*)&dst,len);
		} else {
			dst = ori;
		}
	}
	//fprintf(stderr, "IcmpMgr: socket SND_BUF %d -> %d\n",ori,dst);

	// set recv buffer
	len = sizeof(ori);
	if(getsockopt(m_nSockRaw,SOL_SOCKET,SO_RCVBUF,(char*)&ori,(mysocklen_t *)&len) == 0) {
		if(ori <= 1024*1024) {
			dst = 1024*1024;
			len = sizeof(dst);
			setsockopt(m_nSockRaw,SOL_SOCKET,SO_RCVBUF,(char*)&dst,len);
		} else {
			dst = ori;
		}
	}
	//fprintf(stderr, "IcmpMgr: socket RCV_BUF %d -> %d\n",ori,dst);
	return m_nSockRaw;
}

void CIcmpMgr6::Close()
{
    if(m_nSockRaw >= 0) {
#ifdef _WIN32
       	closesocket(m_nSockRaw);
#else
       	close(m_nSockRaw);
#endif
       	m_nSockRaw = -1;
    }
}

void CIcmpMgr6::FillData(char* data, int datasize, int icmpproto)
{
	switch(icmpproto) {
		case IPPROTO_ICMP:
			FillV4Data(data, datasize);
			break;

		case IPPROTO_ICMPV6:
			FillV6Data(data, datasize);
			break;

		default:
			fprintf(stderr, "CIcmpMgr6::FillData ICMP Version Not Supported\n");
	}
}

void CIcmpMgr6::FillV4Data(char* data, int datasize)
{
    IcmpHeader *hdr;
    char *datapart;

    hdr = (IcmpHeader*)data;

    hdr->i_type = ICMPMGR_ECHO_REQUEST;
    hdr->i_code = 0;
    hdr->i_id = m_nIdent; // (unsigned short)ICMPMGR_IDENT;
    hdr->i_cksum = 0;
    hdr->i_seq = 0;

    datapart = data + sizeof(IcmpHeader);
    //
    // Place some junk in the buffer.
    //
    memset(datapart,'E', datasize - sizeof(IcmpHeader));
}

void CIcmpMgr6::FillV6Data(char* data, int datasize)
{
	struct icmp6_hdr *hdr;
    char *datapart;

    hdr = (struct icmp6_hdr*)data;

    hdr->icmp6_type = ICMP6_ECHO_REQUEST;
    hdr->icmp6_code = 0;
    hdr->icmp6_id = m_nIdent; // (unsigned short)ICMPMGR_IDENT;
    hdr->icmp6_cksum = 0;
    hdr->icmp6_seq = 0;

    datapart = data + sizeof(struct icmp6_hdr);
    //
    // Place some junk in the buffer.
    //
    memset(datapart,'E', datasize - sizeof(struct icmp6_hdr));
}

void CIcmpMgr6::SetIcmp6Filter(int mytype)
{
	int on = 1;

#ifdef _WIN32
	//WSAIoctl(m_nSockRaw, SIO_RCVALL, NULL, 0, NULL, 
		//0, NULL, NULL, NULL);
#endif

	if(mytype == ICMPMGR_MYTYPE_TRACE) { // Filtering for TRACE

		if (m_nVerbose == 0) {
			/*
#if !defined(_WIN32) && !defined(_UNIXWARE)
			struct icmp6_filter myfilt;
			ICMP6_FILTER_SETBLOCKALL(&myfilt);
			ICMP6_FILTER_SETPASS(ICMP6_TIME_EXCEEDED, &myfilt);
			ICMP6_FILTER_SETPASS(ICMP6_DST_UNREACH, &myfilt);
			ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &myfilt);
			setsockopt(m_nSockRaw, IPPROTO_IPV6, ICMP6_FILTER, &myfilt, sizeof(myfilt));
#endif
*/
		}
	} else { // Filtering for PING/FIND

		if (m_nVerbose == 0) {
			/*
#if !defined(_WIN32) && !defined(_UNIXWARE)
			// install a filter that only passes ICMP6_ECHO_REPLY unless verbose 
			struct icmp6_filter myfilt;
			ICMP6_FILTER_SETBLOCKALL(&myfilt);
			ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &myfilt);
			setsockopt(m_nSockRaw, IPPROTO_IPV6, ICMP6_FILTER, &myfilt, sizeof(myfilt));
			// ignore error return; the filter is an optimization 
#endif
		*/
		}

		/* ignore error returned below; we just won't receive the hop limit */
#ifdef IPV6_RECVHOPLIMIT
		/* RFC 3542 */
		setsockopt(m_nSockRaw, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, (char*) &on, sizeof(on));
#elif IPV6_HOPLIMIT
		setsockopt(m_nSockRaw, IPPROTO_IPV6, IPV6_HOPLIMIT, (char*) &on, sizeof(on));
#endif
	}
}

int CIcmpMgr6::CompAddr(struct sockaddr* sa1, struct sockaddr* sa2, mysocklen_t salen)
{
	if (sa1->sa_family != sa2->sa_family) {
		return -1;
	}

	switch (sa1->sa_family) {
		case AF_INET:
			return(memcmp( &((struct sockaddr_in *) sa1)->sin_addr,
						&((struct sockaddr_in *) sa2)->sin_addr,
						sizeof(struct in_addr)));

		case AF_INET6:
			return(memcmp( &((struct sockaddr_in6 *) sa1)->sin6_addr,
						&((struct sockaddr_in6 *) sa2)->sin6_addr,
						sizeof(struct in6_addr)));

		default:
			return -1;		/* no idea what to compare here ? */
	}
	return -1;
}

unsigned short CIcmpMgr6::Checksum(unsigned short* buffer,int size)
{
    unsigned long cksum=0;

    while(size >1) 
    {
       	cksum+=*buffer++;
       	size -=sizeof(unsigned short);
    }
  
    if(size ) 
    {
       	cksum += *(unsigned char*)buffer;
    }

    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16);
    return (unsigned short)(~cksum);
}

// ICMP6 checksum (IPv6 suedo header) - Win32
unsigned short CIcmpMgr6::
ChecksumIcmp6(int s, char *icmppacket, int icmplen, struct sockaddr_in6& dest)
{
	int ret=0;
#ifdef _WIN32
    SOCKADDR_STORAGE localif;
    DWORD            bytes;
    char             tmp[ICMPMGR_MAX_PACKET] = {'\0'},
                    *ptr=NULL,
                     proto=0;
    int              rc, total, length, i;

    // Find out which local interface for the destination
    rc = WSAIoctl(
            s,
            SIO_ROUTING_INTERFACE_QUERY,
            &dest,
            (DWORD) sizeof(dest),
            (SOCKADDR *) &localif,
            (DWORD) sizeof(localif),
           &bytes,
            NULL,
            NULL
            );
    if (rc == SOCKET_ERROR)
    {
        fprintf(stderr, "WSAIoctl failed: %d\n", WSAGetLastError());
        return 0xFFFF;
    }

    // We use a temporary buffer to calculate the pseudo header. 
    ptr = tmp;
    total = 0;

    // Copy source address
    memcpy(ptr, &((SOCKADDR_IN6 *)&localif)->sin6_addr, sizeof(struct in6_addr));
    ptr   += sizeof(struct in6_addr);
    total += sizeof(struct in6_addr);

    // Copy destination address
    memcpy(ptr, &((SOCKADDR_IN6 *)&dest)->sin6_addr, sizeof(struct in6_addr));
    ptr   += sizeof(struct in6_addr);
    total += sizeof(struct in6_addr);

    // Copy ICMP packet length
    length = htonl(icmplen);

    memcpy(ptr, &length, sizeof(length));
    ptr   += sizeof(length);
    total += sizeof(length);

    // Zero the 3 bytes
    memset(ptr, 0, 3);
    ptr   += 3;
    total += 3;

    // Copy next hop header
    proto = IPPROTO_ICMPV6;

    memcpy(ptr, &proto, sizeof(proto));
    ptr   += sizeof(proto);
    total += sizeof(proto);

    // Copy the ICMP header and payload
    memcpy(ptr, icmppacket, icmplen);
    ptr   += icmplen;
    total += icmplen;

    for(i=0; i < icmplen%2 ;i++)
    {
        *ptr = 0;
        ptr++;
        total++;
    }
    
    ret = Checksum((USHORT *)tmp, total);
#endif
	return ret;
}

int CIcmpMgr6::DecodeResp(char* buf, int bytes,
	struct sockaddr* from,long& ts,unsigned short& seqno,int mytype)
{
	int nRet = 0;
	if(from->sa_family == AF_INET) {
		nRet = DecodeResp(buf, bytes, (struct sockaddr_in*)from, ts, seqno, mytype);
	} else if(from->sa_family == AF_INET6) {
		nRet = DecodeResp(buf, bytes, (struct sockaddr_in6*)from, ts, seqno, mytype);
	}
	return nRet;
}

int CIcmpMgr6::DecodeResp(char* buf, int bytes,
	struct sockaddr_in* from,long& ts,unsigned short& seqno,int mytype)
{
	//{{{
    IpHeader *iphdr;
    IcmpHeader *icmphdr;
    unsigned short iphdrlen;
	int nRet = 0;
	int	ttl = -1;

	int myport = 0;
	char myip[40];
	memset(myip, 0, sizeof(myip));

	inet_ntop(AF_INET, &(from->sin_addr), myip, sizeof(myip));
	myport = ntohs(from->sin_port);

	int icmplen;

    iphdr = (IpHeader *)buf;
    iphdrlen = iphdr->ip__hl * 4 ; // number of 32-bit words *4 = bytes

	icmplen = bytes - iphdrlen;
	//hlen1 = iphdr->ip__hl << 2;

    if (bytes  < iphdrlen + ICMPMGR_MIN) 
    {
		nRet = ICMPMGR_STATUS_TOOFEW;
		return nRet; 
    }

	ttl = iphdr->ip__ttl; // ttl
    // get the icmp header
    icmphdr = (IcmpHeader*)(buf + iphdrlen);
	//printf("DecodeResponse: type = %d id = %d, seqno = %d ip = %s\n",
		//icmphdr->i_type,icmphdr->i_id,icmphdr->i_seq,inet_ntoa(from->sin_addr));

	if(mytype == ICMPMGR_MYTYPE_TRACE) { // TRACE: 2007.1.26. by hjson
		if (m_nVerbose) {
			int idx = 0;
			int len = sizeof(idx);
			getsockopt(m_nSockRaw, IPPROTO_IP, IP_TTL, (char*)&idx, (mysocklen_t *)&len);		
			printf("%2d (from %s:%d: type = %d, code = %d)\n",
					idx, myip, myport, icmphdr->i_type, icmphdr->i_code);
		}
        if(icmphdr->i_type != ICMPMGR_DST_UNREACH 
                && icmphdr->i_type != ICMPMGR_TIME_EXCEEDED 
                && icmphdr->i_type != ICMPMGR_ECHO_REPLY) return ICMPMGR_STATUS_OTHER;
        if(icmphdr->i_type == ICMPMGR_DST_UNREACH) {
            if(icmphdr->i_code == 3) 
                return ICMPMGR_STATUS_PORT_UNREACH;
            else
                return ICMPMGR_STATUS_OTHER;
        } else if(icmphdr->i_type == ICMPMGR_TIME_EXCEEDED) nRet = ICMPMGR_STATUS_TMXCEED_TRANS;
        else if(icmphdr->i_type == ICMPMGR_ECHO_REPLY) {
            if(icmphdr->i_id != m_nIdent) {
                return ICMPMGR_STATUS_OTHER;
            }
            nRet = ICMPMGR_STATUS_OK;
        }
	} else { // PING/FIND
		// modi by lordang 2010.09.13
		if(icmphdr->i_type != ICMPMGR_ECHO_REPLY
				|| icmphdr->i_id != m_nIdent) {

			if (m_nVerbose) {
				fprintf(stdout, "%d bytes from %s:%d: type = %d, code = %d\n"
						,bytes, myip, myport, icmphdr->i_type, icmphdr->i_code);
			}
			return nRet;
		}
        nRet = ICMPMGR_STATUS_OK;
	}

#ifdef _WIN32
    m_tv2 = GetTickCount();
	ts = m_tv2 - m_tv1;
#else
	struct timeval tv;
	gettimeofday(&m_tv2,NULL);
	tv.tv_sec = m_tv2.tv_sec - m_tv1.tv_sec;
	tv.tv_usec = m_tv2.tv_usec - m_tv1.tv_usec;
	if(tv.tv_usec < 0) {
		--tv.tv_sec;
		tv.tv_usec += 1000000;
	}
	ts = 1000*tv.tv_sec + (long)(tv.tv_usec/1000.0);
#endif
    if(ts < 0) ts = 1;
    seqno = icmphdr->i_seq;

	if (icmphdr->i_type == ICMPMGR_ECHO_REPLY && m_nVerbose) {
		printf("%d bytes from %s:%d: seq=%u, ttl=",
				bytes, myip, myport, icmphdr->i_seq);
		if (ttl == -1)
			printf("???");	/* ancillary data missing */
		else
			printf("%d", ttl);
		printf(", rtt=%ld ms\n", ts);
	}
	//}}}
    return nRet;
}

int CIcmpMgr6::DecodeResp(char* buf, int bytes,
	struct sockaddr_in6* from,long& ts,unsigned short& seqno,int mytype)
{
	//{{{
    icmp6_hdr *icmp6;
	
	int	hlim=0;
	int nRet = 0;
	int myport = 0;
	char myip[40];
	memset(myip, 0, sizeof(myip));

	inet_ntop(AF_INET6, &from->sin6_addr, myip, sizeof(myip));
	myport = ntohs(from->sin6_port);

    // get the icmp6 header
	icmp6 = (struct icmp6_hdr *)buf; 

    if (bytes  < ICMPMGR_MIN) 
    {
		nRet = ICMPMGR_STATUS_TOOFEW;
		return nRet; 
    }

	if(mytype == ICMPMGR_MYTYPE_TRACE) { // TRACE: 2007.1.26. by hjson
		if (m_nVerbose) {
			int idx = 0;
			int len = sizeof(idx);			
			getsockopt(m_nSockRaw, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
					(char*)&idx, (mysocklen_t *)&len);
			printf("%2d (from %s:%d: type = %d, code = %d)\n",
					idx, myip, myport, icmp6->icmp6_type, icmp6->icmp6_code);
		}

        if(icmp6->icmp6_type != ICMP6_DST_UNREACH 
                && icmp6->icmp6_type != ICMP6_TIME_EXCEEDED 
                && icmp6->icmp6_type != ICMP6_ECHO_REPLY) 
			return ICMPMGR_STATUS_OTHER;
        if(icmp6->icmp6_type == ICMP6_DST_UNREACH) {
            if(icmp6->icmp6_code == ICMP6_DST_UNREACH_NOPORT) 
                return ICMPMGR_STATUS_PORT_UNREACH;
            else
                return ICMPMGR_STATUS_OTHER;
        } else if(icmp6->icmp6_type == ICMP6_TIME_EXCEEDED) 
			nRet = ICMPMGR_STATUS_TMXCEED_TRANS;
        else if(icmp6->icmp6_type == ICMP6_ECHO_REPLY) {
            if(icmp6->icmp6_id != m_nIdent) {
                return ICMPMGR_STATUS_OTHER;
            }
            nRet = ICMPMGR_STATUS_OK;
        }

	} else { // PING/FIND
		if (icmp6->icmp6_type == ICMP6_ECHO_REPLY) {
			if (icmp6->icmp6_id != m_nIdent)
				return nRet;		/* not a response to our ECHO_REQUEST */
			if (bytes < 16)
				return nRet;		/* not enough data to use */

			hlim = -1;
#if 0 // hlim 필요없어서 주석처리함 hop을 지원하지 않는 OS 존재 (2010.12.8)
#ifdef _WIN32
			WSACMSGHDR *cmsg;
			WASMSG *msg = (WASMSG*) mymsg;
			if(msg != NULL) {
				cmsg = WSA_CMSG_FIRSTHDR(msg);
				if(cmsg) {
					if (cmsg->cmsg_level == IPPROTO_IPV6 &&
							cmsg->cmsg_type == IPV6_HOPLIMIT) {
						hlim = *(unsigned int *)WSA_CMSG_DATA(cmsg);						
					}
				}
			}
#else
			struct cmsghdr *cmsg;
			struct msghdr *msg = (msghdr*) mymsg;
			if(msg != NULL) {
				for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
					if (cmsg->cmsg_level == IPPROTO_IPV6 &&
							cmsg->cmsg_type == IPV6_HOPLIMIT) {
						hlim = *(unsigned int *)CMSG_DATA(cmsg);
						break;
					}
				}
			}
#endif
#endif // #if 0
            nRet = ICMPMGR_STATUS_OK;

		} else {
			if (m_nVerbose) {
				fprintf(stdout, "%d bytes from %s:%d: type = %d, code = %d\n"
						,bytes, myip, myport, icmp6->icmp6_type, icmp6->icmp6_code);
			}
			return nRet;
		}
	}

#ifdef _WIN32
    m_tv2 = GetTickCount();
	ts = m_tv2 - m_tv1;
#else
	struct timeval tv;
	gettimeofday(&m_tv2,NULL);
	tv.tv_sec = m_tv2.tv_sec - m_tv1.tv_sec;
	tv.tv_usec = m_tv2.tv_usec - m_tv1.tv_usec;
	if(tv.tv_usec < 0) {
		--tv.tv_sec;
		tv.tv_usec += 1000000;
	}
	ts = 1000*tv.tv_sec + (long)(tv.tv_usec/1000.0);
#endif
    if(ts < 0) ts = 1;
    seqno = icmp6->icmp6_seq;

	if (icmp6->icmp6_type == ICMP6_ECHO_REPLY && m_nVerbose) {
		printf("%d bytes from %s:%d: seq=%u, hlim=",
				bytes, myip, myport, icmp6->icmp6_seq);
#if 0 // hlim 필요없어서 주석처리함 
		if (hlim == -1)
			printf("???");	/* ancillary data missing */
		else
			printf("%d", hlim);
#endif
		printf(", rtt=%ld ms\n", ts);
	}
    return nRet;
	//}}}
}

int CIcmpMgr6::SendTo(unsigned short seqno,char* data,int datasize, char *pszAddr)
{
	struct sockaddr *dest = NULL;
	struct sockaddr_in dest4;
	struct sockaddr_in6 dest6;
	//
	struct addrinfo *res = GetAddrInfo(pszAddr);
	//
	if(res) {
		if(res->ai_family == AF_INET) {
			memcpy(&dest4, res->ai_addr, res->ai_addrlen);
			dest = (struct sockaddr*)&dest4;
			datasize += sizeof(IcmpHeader);
		} else if(res->ai_family == AF_INET6) {
			memcpy(&dest6, res->ai_addr, res->ai_addrlen);
			dest = (struct sockaddr*)&dest6;
			datasize += sizeof(icmp6_hdr);
			if(IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)dest)->sin6_addr)) {
				printf("cannot ping IPv4-mapped IPv6 address %s\n", pszAddr);
			}
			// Set Icmp6 Filtering
			SetIcmp6Filter(ICMPMGR_MYTYPE_PING);
		}
	}
	freeaddrinfo(res);
	return SendTo(seqno,data,datasize,dest);
}

int CIcmpMgr6::SendTo(unsigned short seqno,char* data,int datasize,
		struct sockaddr* dest)
{
	int nwrite = 0;
	if(dest->sa_family == AF_INET) {
		nwrite = SendTo(seqno, data, datasize, *(struct sockaddr_in*)dest);
	} else if(dest->sa_family == AF_INET6) {
		nwrite = SendTo(seqno, data, datasize, *(struct sockaddr_in6*)dest);
	}

	return nwrite;
}

int CIcmpMgr6::SendTo(unsigned short seqno,char* data,int datasize,
		struct sockaddr_in& dest)
{
    FillData(data,datasize);
	//
    ((IcmpHeader*)data)->i_cksum = 0;
    ((IcmpHeader*)data)->i_seq = seqno;
    ((IcmpHeader*)data)->timestamp = time(NULL);
#ifdef _WIN32
	m_tv1 = GetTickCount();
#else
    gettimeofday(&m_tv1,NULL);
#endif
    ((IcmpHeader*)data)->i_cksum = Checksum((unsigned short*)data,datasize);

    return sendto(m_nSockRaw,data,datasize,MSG_DONTWAIT,
	    (struct sockaddr*)&dest,sizeof(dest));
}

int CIcmpMgr6::SendTo(unsigned short seqno,char* data,int datasize,
		struct sockaddr_in6& dest)
{
	FillV6Data(data, datasize);

	struct icmp6_hdr	*icmp6;

	icmp6 = (struct icmp6_hdr *) data;
	icmp6->icmp6_seq = seqno;

#ifdef _WIN32
	m_tv1 = GetTickCount();
#else
    gettimeofday(&m_tv1,NULL);
#endif

#ifdef _WIN32
	icmp6->icmp6_cksum = ChecksumIcmp6(m_nSockRaw,data,datasize,dest);
#endif

    return sendto(m_nSockRaw,data,datasize,MSG_DONTWAIT,
	    (struct sockaddr*)&dest,sizeof(dest));
}

int CIcmpMgr6::RecvFrom(char* buf,int len,
	struct sockaddr_in& from,mysocklen_t& fromlen)
{
	int nread;
	nread = recvfrom(m_nSockRaw,buf,len,0, (struct sockaddr*)&from,&fromlen);
	return nread;
}

int CIcmpMgr6::RecvFrom(char* buf,int len,
	struct sockaddr_in6& from,mysocklen_t& fromlen)
{
	int nread;
	nread = recvfrom(m_nSockRaw,buf,len,0, (struct sockaddr*)&from,&fromlen);
	return nread;
}

int CIcmpMgr6::RecvFrom(char* buf,int len,
	struct sockaddr* from,mysocklen_t& fromlen)
{
	int nread = recvfrom(m_nSockRaw,buf,len,0, from,&fromlen);
	return nread;
}

int CIcmpMgr6::RecvMsg(char *recvbuf, struct sockaddr* from, mysocklen_t fromlen)
{
	int nread = 0;
	char ctrbuf[ICMPMGR_MAX_PACKET] = {0};
#ifdef _WIN32
	WSAMSG msg;
	WSABUF wbRecv, wbCtr;

	wbRecv.buf = recvbuf;
	wbRecv.len = ICMPMGR_MAX_PACKET;
	wbCtr.buf = ctrbuf;
	wbCtr.len = sizeof(ctrbuf);

	msg.lpBuffers = &wbRecv;
	msg.dwBufferCount = 1;
	msg.name = (struct sockaddr*)from;
	msg.namelen = fromlen;
	msg.Control = wbCtr;
	msg.dwFlags = 0;

	GUID WSARecvMsg_GUID = WSAID_WSARECVMSG;
	LPFN_WSARECVMSG WSARecvMsg;
	int nResult;
	int errcode = 0;
	DWORD dwRead = 0;

	// Since WSAIoctl is called here in the same function that WSARecvMsg is called,
	// the error code (m_ErrorCode) is ambiguous. For this reason and other reasons
	// (such as performance), WSAIoctl should be called separately. However, for
	// the purposes of providing a sample use of WSARecvMsg, it helps to have the
	// WSAIoctl together with the call to WSARecvMsg.
	nResult = WSAIoctl(m_nSockRaw, SIO_GET_EXTENSION_FUNCTION_POINTER,
		 &WSARecvMsg_GUID, sizeof WSARecvMsg_GUID,
		 &WSARecvMsg, sizeof WSARecvMsg,
		 &dwRead, NULL, NULL);
	if (nResult == SOCKET_ERROR) {
		errcode = WSAGetLastError();
		WSARecvMsg = NULL;
		return 0;
	}

	nResult = WSARecvMsg(m_nSockRaw, &msg, &dwRead, NULL, NULL);
	if (nResult == SOCKET_ERROR) {
		errcode = WSAGetLastError();
		fprintf(stderr, "RecvMsg: Error Code (%d)\n", errcode);
		return 0;
	}
	nread = (int) dwRead;
#else
	struct msghdr msg;
	struct iovec	iov;

	iov.iov_base = recvbuf;
	iov.iov_len = ICMPMGR_MAX_PACKET; 
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = (char*) from; // hpux1123 의 데이터 타입이 caddr_t => (char *)
	msg.msg_namelen = fromlen;
#ifndef _HPUX
	msg.msg_control = ctrbuf;
	msg.msg_controllen = sizeof(ctrbuf);
#endif

	nread = recvmsg(m_nSockRaw, &msg, 0);
#endif
	return nread;
}

int CIcmpMgr6::Find(char* pFrom,char* pTo,int nTimeout,int nRetry)
{
	struct addrinfo	*res = GetAddrInfo(pFrom);
	if(res) {
		if(res->ai_family == AF_INET) {
			m_nRespNum = FindV4(pFrom, pTo, nTimeout, nRetry);
		} else {
			// TODO: IPv6에서는 범위를 어떻게 지정하는지 모름 (2010.11)
		}
	}
	freeaddrinfo(res);

	return m_nRespNum;
}

int CIcmpMgr6::FindV4(char* pFrom,char* pTo,int nTimeout,int nRetry)
{
    // reset responseok
	m_nRespNum = 0;
    for(int i=0;i<ICMPMGR_MAX_NUMIP;i++) {
		m_uReqSendTm[i] = 0; 	// 2008.07.10 by hjson
		m_nReqTry[i] = 0; 	// 2008.07.10 by hjson
		m_nRespOk[i] = 0;
	}
    
    m_nTimeout = nTimeout;
    m_nFrom = ntohl(inet_addr(pFrom));
    m_nTo = ntohl(inet_addr(pTo));

	// from, to and reqnum
	if(m_nTo == 0 && m_nFrom > 0) m_nTo = m_nFrom;

    if(m_nFrom == INADDR_NONE || m_nTo == INADDR_NONE)
    {
		m_nReqNum = 0;
		return 0;
    }

    if((m_nFrom & 0xffffff00) != (m_nTo & 0xffffff00) || m_nFrom > m_nTo) 
    {
		m_nTo = m_nFrom;
		m_nReqNum = 1;
    } else {
		m_nReqNum = m_nTo - m_nFrom + 1;
		if(m_nReqNum > 255) m_nReqNum = 255;
    }

	char data[ICMPMGR_MAX_PACKET], recvbuf[ICMPMGR_MAX_PACKET];
	int datasize = ICMPMGR_DEF_PACKET_SIZE + sizeof(IcmpHeader);
	struct sockaddr_in  from, dest;
	mysocklen_t fromlen = sizeof(from);

	int nread, nret, selfd;
	unsigned short seqno = 0, retno;
	unsigned int ip, indx, next;
	struct timeval wait;
	fd_set rset, wset;
	long ts;
	time_t tNow;

	// added select for wset 2008.07.10 by hjson
    m_nStart = time(NULL);
	next = m_nFrom;
	while(m_nRespNum < m_nReqNum) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(m_nSockRaw,&rset);
		FD_SET(m_nSockRaw,&wset);
		wait.tv_sec = 1; wait.tv_usec = 0;
		selfd = select(m_nSockRaw+1,&rset,&wset,NULL,&wait);

		tNow = time(NULL);

		if(selfd>0 && FD_ISSET(m_nSockRaw,&rset)) {
	       	nread = RecvFrom(recvbuf,ICMPMGR_MAX_PACKET,from,fromlen);
			if(nread > 0) {
				retno = seqno;
				nret = DecodeResp(recvbuf,nread,&from,ts,retno,
						ICMPMGR_MYTYPE_FIND);
				if(ts>=0 && nret==ICMPMGR_STATUS_OK) {
					ip = ntohl(from.sin_addr.s_addr);
					if(m_nFrom<=ip && ip<=m_nTo) {
						indx = ip % ICMPMGR_MAX_NUMIP;
						if(!m_nRespOk[indx]) {
							m_nRespOk[indx] = 1;
							m_sRespIp[indx].ip = from.sin_addr;
							m_sRespIp[indx].ts = ts;
							m_nRespNum++;
						}
					}
				}
			}
		} else if(selfd>0 && FD_ISSET(m_nSockRaw,&wset)) {
			indx = next % ICMPMGR_MAX_NUMIP;
			if(!m_nRespOk[indx] && m_nReqTry[indx]<nRetry && tNow-m_uReqSendTm[indx] >= 1) {
				m_nReqTry[indx]++; 	// 2008.07.10 by hjson
	    		memset(&dest,0,sizeof(dest));
	    		dest.sin_addr.s_addr = htonl(next);
	    		dest.sin_family = AF_INET;

	    		memset(data,0,ICMPMGR_MAX_PACKET);
	    		seqno++;
	    		SendTo(seqno,data,datasize,dest);
				//printf("Sent: %s\n",inet_ntoa(dest.sin_addr));
				m_uReqSendTm[indx] = tNow;
			}
			next++;
			if(next > m_nTo) next = m_nFrom;
#ifdef _WIN32
			Sleep(1); 		// 10 ms => 1ms 2010.06.01 by lordang
#else
			usleep(1000);	// 10 ms => 1ms 2010.06.01 by lordang
#endif
		}

		if((int)(time(NULL)-m_nStart) >= m_nTimeout) break;
	}
  
	return m_nRespNum;
}

long CIcmpMgr6::Ping(char* pAddr,int nTimeout, int nRetry)
{
    long ts = -1;
	if(!pAddr || strlen(pAddr)==0) return ts;

	struct addrinfo	*res = NULL;
    char *pHost = NULL;
	
    struct sockaddr	*dest=NULL, *from=NULL;
    struct sockaddr_in	dest4, from4;
    mysocklen_t fromlen=0;
    struct sockaddr_in6	dest6, from6;
	
	char data[ICMPMGR_MAX_PACKET];
    int datasize = ICMPMGR_DEF_PACKET_SIZE;
    struct timeval wait;
    fd_set rset, wset;
	unsigned short seqno = 0, retno;
    int ret, selfd, okay = 0;
	time_t tNow;
	unsigned int sendtm=0;
	int nReqTry = 0;

#ifdef _AIX
	std::string sHost, sIf;
	if(CheckUnixScopeId(pAddr, sHost, sIf)) {
        pHost = (char*)sHost.c_str();
        m_nScopeId = atoi(sIf.c_str());
	} else {
        pHost = pAddr;
    }
#else
    pHost = pAddr;
#endif

	res = GetAddrInfo(pHost);
	if(res) {
		if(res->ai_family == AF_INET) {
			memcpy(&dest4, res->ai_addr, res->ai_addrlen);
			dest = (struct sockaddr*)&dest4;
			from = (struct sockaddr*)&from4;
			fromlen = sizeof(from4);
			datasize += sizeof(IcmpHeader);  
		} else if(res->ai_family == AF_INET6) {

			memcpy(&dest6, res->ai_addr, res->ai_addrlen);
			if(m_nScopeId > 0) {
#if !defined(_UNIXWARE) && !defined(_AIX)
				dest6.sin6_scope_id = m_nScopeId;
#endif
			}

			dest = (struct sockaddr*)&dest6;
			from = (struct sockaddr*)&from6;
			fromlen = sizeof(from6);
			datasize += sizeof(icmp6_hdr);

			if(IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)dest)->sin6_addr)) {
				printf("cannot ping IPv4-mapped IPv6 address\n");
				freeaddrinfo(res);
				return -1;
			}

			// Set Icmp6 Filtering
			SetIcmp6Filter(ICMPMGR_MYTYPE_PING);
		}
	} else {
		freeaddrinfo(res);
		return ts;
	}
	freeaddrinfo(res);

	memset(data,0,ICMPMGR_MAX_PACKET);

    m_nStart = time(NULL);

	while(!okay) {
    	FD_ZERO(&rset);
    	FD_ZERO(&wset);
    	FD_SET(m_nSockRaw,&rset);
    	FD_SET(m_nSockRaw,&wset);
    	wait.tv_sec = 1; wait.tv_usec = 0;
       	selfd = select(m_nSockRaw+1,&rset,&wset,NULL,&wait);
		
		tNow = time(NULL);
		if(selfd>0 && FD_ISSET(m_nSockRaw,&rset)) {
		
			ret = RecvMsg(data, from, fromlen);

	    	if(ret >0 && CompAddr(dest, from, fromlen) == 0) {
				retno = seqno;
		    	ret = DecodeResp(data,ret,from,ts,retno,ICMPMGR_MYTYPE_PING);
		    	if(seqno>=retno && ret==ICMPMGR_STATUS_OK) okay = 1;
				//printf("ret = %d seq %d ret %d okay = %d\n",
				//	ret,seqno,retno,okay);
	    	}
       	} else if(selfd>0 && FD_ISSET(m_nSockRaw,&wset) && nReqTry < nRetry && tNow - sendtm >= 1) {
			seqno++;
			SendTo(seqno,data,datasize,dest);

			sendtm = tNow;
			nReqTry++;
		}
#ifdef _WIN32
		Sleep(1); 		// 10 ms => 1ms 2010.06.01 by lordang
#else
		usleep(1000);	// 10 ms => 1ms 2010.06.01 by lordang
#endif
		if(time(NULL) - m_nStart >= nTimeout) break;
    }

	return ts;
}

int CIcmpMgr6::Trace(char* pAddr,int nTimeout,int nHop,int nProbe)
{
#ifdef _WIN32
    m_nStart = GetTickCount();
#else
    m_nStart = time(NULL);
#endif
	m_nRespNum = 0;

    int i,j,ret;
    fd_set fds;
    long ts = -1;

	//
	struct addrinfo	*res;
    struct sockaddr	*dest=NULL, *from=NULL, *lastaddr=NULL;
    struct sockaddr_in	dest4, from4, lastaddr4;
    mysocklen_t fromlen;
    struct sockaddr_in6	dest6, from6, lastaddr6;
	
	char data[ICMPMGR_MAX_PACKET];
    int datasize = ICMPMGR_DEF_PACKET_SIZE;
    struct timeval wait;
	unsigned short seqno = 0, retno;

	res = GetAddrInfo(pAddr);

	if(res) {
		if(res->ai_family == AF_INET) {
			memcpy(&dest4, res->ai_addr, res->ai_addrlen);
			dest = (struct sockaddr*)&dest4;
			from = (struct sockaddr*)&from4;
			fromlen = sizeof(from4);
			datasize += sizeof(IcmpHeader);  
			lastaddr = (struct sockaddr*)&lastaddr4;
		} else if(res->ai_family == AF_INET6) {
			memcpy(&dest6, res->ai_addr, res->ai_addrlen);
			dest = (struct sockaddr*)&dest6;
			from = (struct sockaddr*)&from6;
			fromlen = sizeof(from6);
			datasize += sizeof(icmp6_hdr);  
			lastaddr = (struct sockaddr*)&lastaddr6;		
			if(IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)dest)->sin6_addr)) {
				freeaddrinfo(res);
				printf("cannot ping IPv4-mapped IPv6 address\n");
				return -1;
			}
			// Set Icmp6 Filtering
			SetIcmp6Filter(ICMPMGR_MYTYPE_TRACE);
		}
	} else {
		freeaddrinfo(res);
		return -1;
	}
	freeaddrinfo(res);

	memset(data,0,ICMPMGR_MAX_PACKET);
	memset(lastaddr, 0x00, fromlen);

    for(i=1;i<nHop;i++) {
	   	ts = -1;
       	for(j=0;j<nProbe;j++) {
	    	// send
			if(dest->sa_family == AF_INET) {
				setsockopt(m_nSockRaw, IPPROTO_IP, IP_TTL, (char*)&i, sizeof(i));
				//printf("ttl = %d\n", i);
			} else if(dest->sa_family == AF_INET6) {
				setsockopt(m_nSockRaw, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (char*)&i, sizeof(i));
				//printf("hop limit = %d\n", i);
			}
			seqno++;
	    	SendTo(seqno,data,datasize,dest);
	    	// recv
	    	FD_ZERO(&fds);
	    	FD_SET(m_nSockRaw,&fds);
	    	wait.tv_sec = nTimeout;
	    	wait.tv_usec = 0;
	    	if(select(m_nSockRaw+1,&fds,NULL,NULL,&wait) > 0) {
	       		ret = RecvFrom(data,ICMPMGR_MAX_PACKET,from,fromlen);
	       		if(ret > 0) {
					retno = seqno;
					ret = DecodeResp(data,ret,from,ts,retno,
							ICMPMGR_MYTYPE_TRACE);
					if((ret==ICMPMGR_STATUS_TMXCEED_TRANS && CompAddr(from, lastaddr, fromlen) != 0)
                            || (ret==ICMPMGR_STATUS_OK && CompAddr(from, dest, fromlen) == 0)) {

                        memcpy(lastaddr, from, fromlen);
                        if(m_nRespNum < ICMPMGR_MAX_NUMIP) {
                            m_nRespOk[m_nRespNum] = 1;
                            if(from->sa_family == AF_INET) {
                                memcpy(&m_sRespIp[m_nRespNum].ip, 
										&((struct sockaddr_in*)from)->sin_addr, 
										sizeof(m_sRespIp[m_nRespNum].ip));
								m_sRespIp[m_nRespNum].ts = ts;
                            } else if(from->sa_family == AF_INET6) {
                                memcpy(&m_sRespIp6[m_nRespNum].ip, 
										&((struct sockaddr_in6*)from)->sin6_addr, 
										sizeof(m_sRespIp6[m_nRespNum].ip));
								m_sRespIp6[m_nRespNum].ts = ts;
                            }
                            m_nRespNum++;
                        }
					}
	       		}
	    	}
			if(ts >= 0) break;
       	}
		if(ts<0) break;
       	if((CompAddr(dest, lastaddr, fromlen) == 0)) break;
    }
    
    return m_nRespNum;
}
