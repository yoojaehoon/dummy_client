
#ifndef _TCPSOCK_H_
#define _TCPSOCK_H_

#include "stdafx.h"

#ifdef _USE_IPV4ONLY 	
#include "tcpsock4.h"

#define CTcpSock 	CTcpSock4
#define STcpSock 	STcpSock4
#define CUdpSock 	CUdpSock4

#else

#include "tcpsock6.h"
#define CTcpSock 	CTcpSock6
#define STcpSock 	STcpSock6
#define CUdpSock 	CUdpSock6

#endif

#endif
