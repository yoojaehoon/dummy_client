/*
   Copyright (c) 2002-2005 BrainzSquare, Inc.
   icmpmgr.cpp - icmpmgr ping
   
   2005.03.04. modified Find, Trace
   FIX ME: trace may be incorrect at some circumstance
 */

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef _AIX
#include <strings.h>
#endif

#include "icmpmgr4.h"

CIcmpMgr4::CIcmpMgr4()
{
    m_nSockRaw = -1;
    m_nRespNum = 0;
    m_nTimeout = 5;
    m_nReqNum = 0; // -1 => 0 by kskim, shnoh 2007.05.31
    m_nStart = 0;

	memset(m_nRespOk,0,sizeof(int)*ICMPMGR_MAX_NUMIP);
	memset(m_sRespIp,0,sizeof(IcmpItem)*ICMPMGR_MAX_NUMIP);
	memset(m_nReqIP,0,sizeof(unsigned long)*ICMPMGR_MAX_NUMIP);
	memset(m_nReqId,0,sizeof(void*)*ICMPMGR_MAX_NUMIP);
	memset(m_nReqTry,0,sizeof(int)*ICMPMGR_MAX_NUMIP);
	memset(m_uReqSendTm,0,sizeof(unsigned int)*ICMPMGR_MAX_NUMIP);
	// added by kskim. 2010.4.6
	m_nIdent = (unsigned short) ICMPMGR_IDENT;
}

CIcmpMgr4::~CIcmpMgr4()
{
    Close();
}

// added by kskim. 20070409
int CIcmpMgr4::Ping(int nTimeout,int nRetry)
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
				nret = DecodeResp(recvbuf,nread,from,ts,
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

// added by kskim. 20070409
int CIcmpMgr4::Hash(unsigned int addr,unsigned short seqno)
{
	for(int i=0;i<m_nReqNum;i++) {
		if(m_nReqIP[i] == addr && m_sRespIp[i].seq == seqno) return i;
	}
	return -1;
}

int CIcmpMgr4::Add(unsigned int uIP,void* pId)
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

int CIcmpMgr4::Add(char* pHost,void* pId)
{
	int ret=0;
	if(m_nReqNum < ICMPMGR_MAX_NUMIP) {
		ret = 1;
		m_nReqIP[m_nReqNum] = IP2Num(pHost);
		m_nReqId[m_nReqNum] = pId; 		// 2008.07.10 by hjson
		m_nReqNum++;
	}
	return ret;
}

// added by kskim. 20070425
int CIcmpMgr4::Clear()
{
	m_nReqNum = 0;
	return 0;
}

int CIcmpMgr4::Open()
{
    m_nSockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (m_nSockRaw < 0) {
		printf("CIcmpMgr4::Open failed to create socket\n");
		return -1;
	}

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    bind(m_nSockRaw,(struct sockaddr*)&addr,sizeof(addr));

	int ori, dst,  ret;
	mysocklen_t len;

	// set send buffer
	len = sizeof(ori);
	if(getsockopt(m_nSockRaw,SOL_SOCKET,SO_SNDBUF,(char*)&ori,(mysocklen_t *)&len) == 0) {
		if(ori <= 1024*1024) {
			dst = 1024*1024;
			len = sizeof(dst);
			ret = setsockopt(m_nSockRaw,SOL_SOCKET,SO_SNDBUF,(char*)&dst,len);
		}
	}

	// set recv buffer
	len = sizeof(ori);
	if(getsockopt(m_nSockRaw,SOL_SOCKET,SO_RCVBUF,(char*)&ori,(mysocklen_t *)&len) == 0) {
		if(ori <= 1024*1024) {
			dst = 1024*1024;
			len = sizeof(dst);
			setsockopt(m_nSockRaw,SOL_SOCKET,SO_RCVBUF,(char*)&dst,len);
		}
	}
	return m_nSockRaw;
}

void CIcmpMgr4::Close()
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

void CIcmpMgr4::FillData(char* data, int datasize)
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

unsigned short CIcmpMgr4::Checksum(unsigned short* buffer,int size)
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

int CIcmpMgr4::DecodeResp(char* buf, int bytes,
	struct sockaddr_in& from,long& ts,unsigned short& seqno,int mytype)
{
    IpHeader *iphdr;
    IcmpHeader *icmphdr;
    unsigned short iphdrlen;
	int nRet = 0;

    iphdr = (IpHeader *)buf;
    iphdrlen = iphdr->ip__hl * 4 ; // number of 32-bit words *4 = bytes

    if (bytes  < iphdrlen + ICMPMGR_MIN) 
    {
		nRet = ICMPMGR_STATUS_TOOFEW;
		return nRet; 
    }

    // get the icmp header
    icmphdr = (IcmpHeader*)(buf + iphdrlen);
	//printf("DecodeResponse: type = %d id = %d, seqno = %d ip = %s\n",
	//	icmphdr->i_type,icmphdr->i_id,icmphdr->i_seq,inet_ntoa(from.sin_addr));

	if(mytype == ICMPMGR_MYTYPE_TRACE) { // TRACE: 2007.1.26. by hjson
		if(
			icmphdr->i_type == 11 //ICMP_ECHOREPLY
			&& icmphdr->i_id == m_nIdent // (unsigned short)ICMPMGR_IDENT
			&& icmphdr->i_seq == seqno) return nRet;
	} else { // PING/FIND
		//if(icmphdr->i_id>0 && icmphdr->i_id!=ICMPMGR_IDENT) return nRet;
		if(icmphdr->i_id>0 && icmphdr->i_id!=m_nIdent) return nRet;
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

#ifndef _SOLARIS
	// commented by shnoh 10.29
	/*
    if (icmphdr->i_type != ICMPMGR_ECHO_REPLY) 
    {
		nRet = ICMPMGR_STATUS_NOECHO;
		return nRet;
    }
    if (icmphdr->i_id != (unsigned short)ICMPMGR_IDENT) 
    {
		nRet = ICMPMGR_STATUS_OTHER;
       	return nRet;
    }
	*/
#endif

    nRet = ICMPMGR_STATUS_OK;
    return nRet;
}

int CIcmpMgr4::SendTo(unsigned short seqno,char* data,int datasize,
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
    ((IcmpHeader*)data)->i_cksum =
       	Checksum((unsigned short*)data,datasize);

    return sendto(m_nSockRaw,data,datasize,MSG_DONTWAIT,
	    (struct sockaddr*)&dest,sizeof(dest));
}

int CIcmpMgr4::RecvFrom(char* buf,int len,
	struct sockaddr_in& from,mysocklen_t& fromlen)
{
	int nread = recvfrom(m_nSockRaw,buf,len,0,
		(struct sockaddr*)&from,&fromlen);
	return nread;
}

int CIcmpMgr4::Find(char* pFrom,char* pTo,int nTimeout,int nRetry)
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
				nret = DecodeResp(recvbuf,nread,from,ts,retno,
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
			Sleep(1); 		// _POSCO 10 ms => 1ms
#else
			usleep(1000);	// _POSCO 10 ms => 1ms
#endif
		}

		if((int)(time(NULL)-m_nStart) >= m_nTimeout) break;
	}
  
	return m_nRespNum;
}

long CIcmpMgr4::Ping(char* pAddr,int nTimeout, int nRetry)
{
    long ts = -1;
	if(!pAddr || strlen(pAddr)==0) return ts;
    
    m_nStart = time(NULL);

    struct sockaddr_in	dest, from;
    mysocklen_t fromlen = sizeof(from);
    struct hostent* hp = NULL;
    unsigned long addr;

    if((addr=inet_addr(pAddr)) == INADDR_NONE) {
		hp = gethostbyname(pAddr);
		if(!hp) return ts;
    }
    memset(&dest,0,sizeof(dest));
    if(hp) {
		memcpy(&(dest.sin_addr),hp->h_addr,hp->h_length);
       	dest.sin_family = hp->h_addrtype;
       	addr = dest.sin_addr.s_addr;
    } else {
       	dest.sin_addr.s_addr = addr;
       	dest.sin_family = AF_INET;
    }

    char data[ICMPMGR_MAX_PACKET];
    int datasize = ICMPMGR_DEF_PACKET_SIZE;
    struct timeval wait;
    fd_set rset, wset;
	unsigned short seqno = 0, retno;
    int ret, selfd, okay = 0;
	time_t tNow;
	unsigned int sendtm=0;
	int nReqTry = 0;

    datasize += sizeof(IcmpHeader);  
    memset(data,0,ICMPMGR_MAX_PACKET);

	// added select for wset 2008.07.10 by hjson
	while(!okay) {
    	FD_ZERO(&rset);
    	FD_ZERO(&wset);
    	FD_SET(m_nSockRaw,&rset);
    	FD_SET(m_nSockRaw,&wset);
    	wait.tv_sec = 1; wait.tv_usec = 0;
       	selfd = select(m_nSockRaw+1,&rset,&wset,NULL,&wait);

		tNow = time(NULL);

		if(selfd>0 && FD_ISSET(m_nSockRaw,&rset)) {
	    	ret = RecvFrom(data,ICMPMGR_MAX_PACKET,from,fromlen);
			//printf("Recv From %s\n",inet_ntoa(from.sin_addr));
	    	if(ret >0 && addr==from.sin_addr.s_addr) {
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
		Sleep(1); 		// _POSCO 10 ms => 1ms
#else
		usleep(1000);	// _POSCO 10 ms => 1ms
#endif

		if(time(NULL) - m_nStart >= nTimeout) break;
    }
    
    return ts;
}

#ifdef _WIN32

typedef HANDLE (WINAPI IcmpCreateFile)(VOID);
typedef BOOL (WINAPI IcmpCloseHandle)(HANDLE IcmpHandle);
typedef DWORD (WINAPI IcmpSendEcho)(HANDLE IcmpHandle,
		unsigned long DestinationAddress,
		LPVOID RequestData, WORD RequestSize,
		IP_OPTION_INFORMATION_MY* RequestOptions,
	   	LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);

class CWinIcmp {
public:
	BOOL		m_bInit;
	HINSTANCE	sm_hIcmp;
	IcmpCreateFile* sm_pIcmpCreateFile;
	IcmpSendEcho* sm_pIcmpSendEcho;
	IcmpCloseHandle* sm_pIcmpCloseHandle;
	
	CWinIcmp();
	~CWinIcmp();
	BOOL Init();
};

CWinIcmp::CWinIcmp()
{
	m_bInit = FALSE;
	sm_hIcmp = NULL;
}

CWinIcmp::~CWinIcmp()
{
	if(sm_hIcmp) {
		FreeLibrary(sm_hIcmp);
		sm_hIcmp = NULL;
	}
}

BOOL CWinIcmp::Init()
{
	if(sm_hIcmp) return m_bInit;
	
	sm_hIcmp = LoadLibrary("ICMP.DLL");
	if(sm_hIcmp == NULL) return m_bInit;
	
	sm_pIcmpCreateFile = (IcmpCreateFile*)GetProcAddress(sm_hIcmp,
			"IcmpCreateFile");
	sm_pIcmpSendEcho = (IcmpSendEcho*)GetProcAddress(sm_hIcmp,
			"IcmpSendEcho" );
	sm_pIcmpCloseHandle = (IcmpCloseHandle*)GetProcAddress(sm_hIcmp,
			"IcmpCloseHandle");
	m_bInit = (sm_pIcmpCreateFile && sm_pIcmpSendEcho && sm_pIcmpCloseHandle);
	return m_bInit;
}

static CWinIcmp g_winIcmp;

int CIcmpMgr4::Trace(char* pAddr,int nTimeout,int nHop,int nProbe)
{
#ifdef _WIN32
    m_nStart = GetTickCount();
#else
    m_nStart = time(NULL);
#endif
	m_nRespNum = 0;

    // address
    unsigned long addr = inet_addr(pAddr);
    struct hostent* hp = NULL;
    if(addr == INADDR_NONE) {
       	hp = gethostbyname(pAddr);
       	if(!hp) return 0;

       	struct sockaddr_in dest;
       	memcpy(&(dest.sin_addr),hp->h_addr,hp->h_length);
       	addr = dest.sin_addr.s_addr;
    }

    // open socket
    if(!g_winIcmp.Init()) return 0;
 
    // handle
    HANDLE hIP = g_winIcmp.sm_pIcmpCreateFile();
    if(hIP == NULL) return 0;

    IP_OPTION_INFORMATION_MY opInfo;
    memset(&opInfo,0,sizeof(opInfo));

    unsigned char data[ICMPMGR_MAX_PACKET];
    int nsize = ICMPMGR_DEF_PACKET_SIZE;
    memset(data,'E',sizeof(data));
	
    int nReplySize = sizeof(ICMP_ECHO_REPLY_MY)+nsize;
    unsigned char pReply[ICMPMGR_MAX_PACKET];
    ICMP_ECHO_REPLY_MY* pEchoReply = (ICMP_ECHO_REPLY_MY*)pReply;

    unsigned long lastaddr;
    int i,j,nread,nth = 0;
    long ts;

    lastaddr = 0;
    for(i=1;i<nHop;i++) {
		ts = -1;
       	for(j=0;j<nProbe;j++) {
       		memset(&opInfo,0,sizeof(opInfo));
       		opInfo.Ttl = i;
			nread = g_winIcmp.sm_pIcmpSendEcho(hIP,addr,data,nsize,&opInfo,
				pReply,nReplySize,nTimeout*1000);
			if(nread == 1) {
				ts = (long)pEchoReply->RoundTripTime;
				if(lastaddr != pEchoReply->Address) {
					lastaddr = pEchoReply->Address;
					// set destination address
					if(m_nRespNum < ICMPMGR_MAX_NUMIP) {
						m_nRespOk[m_nRespNum] = 1;
						m_sRespIp[m_nRespNum].ip.S_un.S_addr = lastaddr;
			       		m_sRespIp[m_nRespNum].ts = ts;
			       		m_nRespNum++;
			    	}
		    	}
			}
       	}
		if(ts < 0 || addr == lastaddr) break;
    }
	
    // close icmp handle
    g_winIcmp.sm_pIcmpCloseHandle(hIP);

    return m_nRespNum;
}

#else	// _WIN32

int CIcmpMgr4::Trace(char* pAddr,int nTimeout,int nHop,int nProbe)
{
#ifdef _WIN32
    m_nStart = GetTickCount();
#else
    m_nStart = time(NULL);
#endif
	m_nRespNum = 0;

    unsigned long addr = inet_addr(pAddr);
    struct hostent* hp = NULL;
    if(addr == INADDR_NONE) {
		hp = gethostbyname(pAddr);
		if(!hp) return 0;
    }

    // send
    struct sockaddr_in	dest, from;
    mysocklen_t fromlen = sizeof(from);

    memset(&dest,0,sizeof(dest));
    if(hp) {
		memcpy(&(dest.sin_addr),hp->h_addr,hp->h_length);
       	dest.sin_family = hp->h_addrtype;
       	addr = dest.sin_addr.s_addr;
    } else {
       	dest.sin_addr.s_addr = addr;
       	dest.sin_family = AF_INET;
    }

    int i,j,ret;
	unsigned short seqno = 0, retno;
    unsigned long lastaddr;
    fd_set fds;
    struct timeval wait;
	char data[ICMPMGR_MAX_PACKET];
    int datasize = ICMPMGR_DEF_PACKET_SIZE;
    //int datasize = sizeof(data); 	// 2007.1.26
    long ts = -1;

	// 2007.1.26 by hjson
    datasize += sizeof(IcmpHeader);  
    memset(data,0,ICMPMGR_MAX_PACKET);
	//

	lastaddr = 0;
    for(i=1;i<nHop;i++) {
	   	ts = -1;
       	for(j=0;j<nProbe;j++) {
	    	// send
       		setsockopt(m_nSockRaw, IPPROTO_IP, IP_TTL, (char*)&i, sizeof(i));
			seqno++;
#ifdef _HJSON
			IpHeader *iphdr = (IpHeader *)data;
			//
			iphdr->ip__v = 4;
			//iphdr->ip__hl = IPVERSION;
			//
			//iphdr->ip__len = ;
			//iphdr->ip__off = ;
			//
			iphdr->ip__ttl = i;
			iphdr->ip__p = IPPROTO_ICMP;
			//iphdr->ip__src = 0;
			iphdr->ip__dst = dest.sin_addr.s_addr;
			//iphdr->ip__sum = 0;
#endif
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
		    		if(ret==ICMPMGR_STATUS_OK) {
		       			if(from.sin_addr.s_addr != lastaddr) {
			    			lastaddr = from.sin_addr.s_addr;
			    			if(m_nRespNum < ICMPMGR_MAX_NUMIP) {
								m_nRespOk[m_nRespNum] = 1;
			       				m_sRespIp[m_nRespNum].ip = from.sin_addr;
			       				m_sRespIp[m_nRespNum].ts = ts;
			       				m_nRespNum++;
			    			}
		       			}
		    		}
	       		}
	    	}
			if(ts > 0) break;
       	}
       	if(ts < 0 || addr == lastaddr) break;
    }
    
    return m_nRespNum;
}

#endif	// _WIN32

