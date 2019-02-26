
#ifndef _ICMPMGR_H_
#define _ICMPMGR_H_

#include "stdafx.h"

#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h> // windows xp WSAMSG
#include <Mstcpip.h>
#include <Iphlpapi.h>
//#include <Icmpapi.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#endif

#include "tcpsock.h"

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT    0
#endif

#ifndef INADDR_NONE
#define INADDR_NONE             (unsigned int)(-1)
#endif

#if defined(_AIX) || defined(_HPUX) || defined(_SOLARIS)
#define _OS_BIG_ENDIAN  1234
#endif

#define ICMPMGR_STATUS_TMXCEED_TRANS	-2
#define ICMPMGR_STATUS_PORT_UNREACH		-1
#define ICMPMGR_STATUS_NONE		0
#define ICMPMGR_STATUS_OK		1
#define ICMPMGR_STATUS_TOOFEW	2
#define ICMPMGR_STATUS_NOECHO	3
#define ICMPMGR_STATUS_OTHER	4

#define ICMPMGR_MAX_NUMIP		256

#ifndef _MYSOCKLEN_T
#define _MYSOCKLEN_T
#if     (_UNIXWARE)
typedef size_t      mysocklen_t;
#elif   (_WIN32 || _SOLARIS==5 || _SOLARIS==6 || _OSF1 || _HPUX)

#if !defined(_XOPEN_SOURCE_EXTENDED) // hpux
typedef int			mysocklen_t;
#else
typedef socklen_t 	mysocklen_t;
#endif

#else
typedef socklen_t   mysocklen_t;
#endif  // _UNIXWARE
#endif

// icmp manager type; 2007.1.27. by hjson
#define ICMPMGR_MYTYPE_NONE 	0
#define ICMPMGR_MYTYPE_PING 	1
#define ICMPMGR_MYTYPE_FIND		2
#define ICMPMGR_MYTYPE_TRACE	3

// by lordang 2010.09.10
#define ICMPMGR_DST_UNREACH 	3
#define ICMPMGR_TIME_EXCEEDED 	11

#define ICMPMGR_ECHO_REQUEST	8
#define ICMPMGR_ECHO_REPLY   	0
#define ICMPMGR_MIN	8 // minimum 8 byte icmpmgr packet (just header)

#define ICMPMGR_IDENT	0x6238

/* The IP header */
typedef struct _iphdr
{
#ifdef	_OS_BIG_ENDIAN 
	unsigned int    ip__v:4;        // Version of IP
	unsigned int    ip__hl:4;       // length of the header
#else
	unsigned int    ip__hl:4;       // length of the header
	unsigned int    ip__v:4;        // Version of IP
#endif
	unsigned char   ip__tos;       // Type of service
	unsigned short  ip__len;          // total length of the packet
	unsigned short  ip__id;       // unique ip__idifier
	unsigned short  ip__off;          // fragment offset field
#define IP__RF      0x8000      /* reserved fragment flag */
#define IP__DF      0x4000      /* dont fragment flag */
#define IP__MF      0x2000      /* more fragments flag */
#define IP__OFFMASK 0x1fff      /* mask for fragmenting bits */
	unsigned char   ip__ttl;        // time to live
	unsigned char   ip__p;         // protocol (TCP, UDP etc)
	unsigned short  ip__sum;      // IP checksum
	unsigned int    ip__src;
	unsigned int    ip__dst;
} IpHeader;

//
// ICMPMGR header
//
typedef struct _icmphdr 
{
    unsigned char i_type;
    unsigned char i_code; /* type sub code */
    unsigned short i_cksum;
    unsigned short i_id;
    unsigned short i_seq;
    /* This is not the std header, but we reserve space for time */
    unsigned long timestamp;
} IcmpHeader;

#ifdef _LINUX
typedef struct _udphdrbsd {
    unsigned short uh_sport;
    unsigned short uh_dport;
    unsigned short uh_ulen;
    unsigned short uh_sum;
} UdpHdrBsd;
#else
typedef struct udphdr	UdpHdrBsd;
#endif

typedef struct _pseudoudphdr {
    struct in_addr	source;
    struct in_addr	dest;
    unsigned char	zero;
    unsigned char	proto;
    unsigned short	length;
} PseudoUdpHdr;

#define STATUS_FAILED	0xFFFF
#define DEF_PACKET_SIZE	32
#define MAX_PACKET	1024

#define ICMPMGR_DEF_PACKET_SIZE		DEF_PACKET_SIZE
#define ICMPMGR_MAX_PACKET			MAX_PACKET

struct IcmpItem {
	struct in_addr	ip;
	long			ts;
	unsigned short  seq; 	// 2008.08.13 by hjson
};

#ifdef _USE_IPV4ONLY 	
#include "icmpmgr4.h"
#define CIcmpMgr 	CIcmpMgr4
#else
#include "icmpmgr6.h"
#define CIcmpMgr 	CIcmpMgr6
#endif

#endif

