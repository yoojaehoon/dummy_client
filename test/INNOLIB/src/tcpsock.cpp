
#include "stdafx.h"

#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _AIX
#include <strings.h>
#endif

#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include "tcpsock.h"

#ifdef _TCP_ECHO_S_TEST
int main(int argc,char* argv[])
{
#ifdef _WIN32   // startup socket; needed ws2_32.lib
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 1, 1 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )  return 0;
#endif

#ifndef _USE_IPV4ONLY
	struct in6_addr sin6_addr;
#endif
	int clisockfd, myport, youport;
	char youip[40];
	char buf[256];
	char *pIp = NULL;

	if(argc == 2) {
		myport = atoi(argv[1]);
	} else if(argc == 3) {
		pIp = argv[1];
		myport = atoi(argv[2]);
	} else {
		printf("Usage: %s [ <host> ] <service or port>\n", argv[0]);
		return 0;
	}

	STcpSock sock;
	if(sock.Open(myport, pIp) < 0) {
		fprintf(stderr, "Open() Failed.(bind, listen error)\n");
		return 0;
	}
	sock.SetTimeout(5);
	while(1) {
		clisockfd = sock.Accept(youip, &youport, sizeof(youip)); 
		if(clisockfd < 0) {
			fprintf(stderr, "Accept Failed\n");
			break;
		}
		fprintf(stdout, "Accepted fd(%d) from %s:%d\n", clisockfd, youip, youport);

		sock.GetPeerName(youip, sizeof(youip), youport, clisockfd);
		unsigned int nip = 0;
#ifndef _USE_IPV4ONLY
		nip = sock.GetPeerAddr(&sin6_addr, clisockfd);
#else
		nip = sock.GetPeerAddr(clisockfd);
#endif
		fprintf(stdout, "GetPeerName:%s:%d\n" , youip, youport);

		CTcpSock sess;
		sess.SetSocket(clisockfd);
		memset(buf, 0x00, sizeof(buf));
		sess.Recv(buf, sizeof(buf));
		fprintf(stdout, "Received msg: [%s]\n", buf);
		sess.Send(buf, sizeof(buf));
		fprintf(stdout, "Sent msg: [%s]\n", buf);
		mysleep(1);
		sess.Close();
	}
    sock.Close();
	return 0;

#ifdef _WIN32   // cleanup socket; needed ws2_32.lib
    WSACleanup();
#endif
}
#endif


#ifdef _TCP_ECHO_C_TEST
int main(int argc,char* argv[])
{
#ifdef _WIN32   // startup socket; needed ws2_32.lib
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 1, 1 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )  return 0;
#endif

	char buf[256];
	CTcpSock sock;
	if(argc < 3) {
		fprintf(stderr, "Usage: %s ip port\n", argv[0]);
		return 0;
	}
	int myport = atoi(argv[2]);
	if(sock.Connect(argv[1], myport) < 0) {
		fprintf(stderr, "Connect Failed\n");
		return 0;
	}
	sock.SetTimeout(5);
	fprintf(stdout, "Connected to %s:%d\n", argv[1], myport);
	memset(buf, 0x00, sizeof(buf));
	fgets(buf, sizeof(buf), stdin);
	
	sock.Send(buf, sizeof(buf));
	fprintf(stdout, "Sent msg: [%s]\n", buf);
	sock.Recv(buf, sizeof(buf));
	fprintf(stdout, "Received msg: [%s]\n", buf);
	sock.Close();
	fprintf(stdout, "Disconnected\n");
	return 0;

#ifdef _WIN32   // cleanup socket; needed ws2_32.lib
    WSACleanup();
#endif
}
#endif

#define _RESOLV_DNS_TEST
#ifdef _RESOLV_DNS_TEST
int main(int argc,char* argv[])
{
#ifdef _WIN32   // startup socket; needed ws2_32.lib
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 1, 1 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )  return 0;
#endif

	char dns[255] = {0}, addrtest[40] = {0};
	char myip[40] = {0};
	int myport;

	if(argc == 2) {
		myport = atoi(argv[1]);
	} else {
		printf("Usage: %s [ <host> ] <service or port>\n", argv[0]);
		return 0;
	}

    CTcpSock sock;
	strncpy(dns, argv[1], sizeof(dns));
#ifndef _USE_IPV4ONLY
	struct sockaddr addr;
	mysocklen_t addrlen;
	int time = sock.ResolvDns(dns, myip, &addr, &addrlen);
	fprintf(stdout, "%dms = DNS (\'%s\') => IP (\'%s\')\n", time, dns, myip);
    SockAddr2IP(addr, addrtest, sizeof(addrtest));
	myport = GetSockAddrPort((struct sockaddr*)&addr);
	fprintf(stdout, "   Returned addr = %s:%d\n", addrtest, myport);
#else
	struct sockaddr_in addr;
	int time = sock.ResolvDns(dns, myip, &addr);
	fprintf(stdout, "%dms = DNS (\'%s\') => IP (\'%s\')\n", time, dns, myip);
	inet_ntop(AF_INET, &addr, addrtest, sizeof(addrtest));
	fprintf(stdout, "   Returned addr = %s:%d\n", addrtest, (int) ntohs(addr.sin_port));
#endif
	time = sock.ResolvDns(dns, myip);
	fprintf(stdout, "Simple: %dms = DNS (\'%s\') => IP (\'%s\')\n", time, dns, myip);

#ifdef _WIN32   // cleanup socket; needed ws2_32.lib
    WSACleanup();
#endif
}
#endif
