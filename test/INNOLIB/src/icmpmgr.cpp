/*!
 * created by kskim. 2010.12.8
 *
 * Ping 체크를 위한 클래스 정의부
 * IP version 4/6를 지원함.
 * ipv6에 대해 전혀 고려되지 않은 노후 OS가 존재하므로 ipv4 전용 클래스를 제공함
 * CIcmpMgr4는 20010.11.23 이전 zenlib의 CIcmpMgr와 완전 동일함
 *
 */

#include "stdafx.h"

#include <ctype.h>
#include <string>

#include "icmpmgr.h"

#if defined(_ICMPMGR_TEST ) && defined(_USE_IPV4ONLY)

#define OPTION_PING				1	
#define OPTION_TRACE			2	
#define OPTION_TEST_PING		3
#define OPTION_TEST_MULTIPING	4
#define OPTION_TEST_TRACE		5
#define OPTION_VERBOSE			6
#define OPTION_TEST_FIND		7

char *g_pDest[256] = {NULL};
int g_nVerbose = 0;

int usage(char *progname)
{
    printf("Usage: %s <OPTIONS> IP [IP]...\n", progname);
	printf("OPTIONS:\n");
    printf("  -p            Ping Single or Multiple Host\n");	
    printf("  -t            Traceroute\n");
    printf("  -v VERIOPTS   Verify ICMP Routine\n");
	printf("                  p:  Verify Ping\n");
	//printf("                  m:  Verify Pinging Multiple Host\n");
	//printf("                  t:  Verify Traceroute\n");
	printf("  -d            Show some verbose description\n");
	printf("  -f            Find device (using icmp protocol)\n");
    printf("\n");
	printf("(CAUTION: Can't do multiping ipv4 and ipv6 addresses simultaneously!)\n");
	printf("\n");
    return 0;
}

// 옵션 구분
int ValidateArgs(int argc, char **argv)
{
    int    i, option, nCnt = 0;
 
    for(i=1; i < argc ;i++)
    {
        if ((argv[i][0] == '-'))
        {
            switch (tolower(argv[i][1]))
            {
                case 'p':        // ping
                    if (i+1 >= argc)
                        usage(argv[0]);
					option = OPTION_PING;                    
                    break;
                case 't':        // trace
                    if (i+1 >= argc)
                        usage(argv[0]);
					option = OPTION_TRACE;                    
					
                    break;
                case 'v':        // verification
                    if (i+1 >= argc)
                        usage(argv[0]);
                    if (argv[i+1][0] == 'p') {
						option = OPTION_TEST_PING;
#if 0
                    } else if (argv[i+1][0] == 'm') {
						option = OPTION_TEST_MULTIPING;
					} else if (argv[i+1][0] == 't') {
						option = OPTION_TEST_TRACE;
#endif
					}
					i++;					                    
                    break;
                case 'f':        // find
                    if (i+1 >= argc)
                        usage(argv[0]);
                    if (argv[i+1][0] == 'f') {
						option = OPTION_TEST_FIND;
					}
					i++;					                    
                    break;
				case 'd':					
					g_nVerbose = 1;					
					break;
                default:
                    usage(argv[0]);
                    break;
            }
        }
        else
        {
            g_pDest[nCnt] = argv[i];
			nCnt++;
        }
    }
    return option;
}
 
#define MAXLINE 255

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define snprintf _snprintf
#endif

int DoTestFind(char *from, char *to)
{
    CIcmpMgr mgr;
	if(mgr.Open()>0) {
		mgr.Find(from, to, 10);
		for(int i=0;i<mgr.m_nReqNum;i++) {
			if(mgr.m_nRespOk[i]) {
				printf("%02d: %s %ld ms, res %d\n",
					i+1,inet_ntoa(mgr.m_sRespIp[i].ip),mgr.m_sRespIp[i].ts,
					mgr.m_nRespOk[i]);
			}
		}
		printf("Response [%d/%d]\n",mgr.m_nRespNum,mgr.m_nReqNum);
	}
	mgr.Close();
	return 0;
}

int DoPing(char *pDest)
{
    CIcmpMgr mgr;
	int	result;
	time_t	stime=time(NULL), etime=0;
	long ts;
    char *pHost = NULL;

    pHost = pDest;

	int res = GetIPVersion(pHost);
	if(res == AF_INET) {
		result = mgr.Open();
	} else if(res == AF_INET6) {
#ifndef _USE_IPV4ONLY
		result = mgr.Open(IPPROTO_ICMPV6);
#endif
	}

#ifdef _WIN32
	DWORD pid = GetCurrentProcessId();
#else
	pid_t pid = getpid();
#endif
	mgr.SetIdent(pid & 0xffff);
	if(result>0) {
		printf("START...\n");
#ifndef _USE_IPV4ONLY
		mgr.SetVerbose(g_nVerbose);
#endif

		ts = mgr.Ping(pDest, 5); // 10.0.0.54

		etime = time(NULL);
		printf("END : %ld ms, time : %ld sec\n", ts, etime-stime);
		if(ts < 0) result = 0;
		else result = 1;
	}
	mgr.Close();
	
	return result;
}

void DoMPing(char **pDest)
{
    CIcmpMgr mgr;
	int	result =0;
	char temp[64] = {0};
	
	for(int i=0; pDest[i]; i++)
	{
		mgr.Add(pDest[i]);
	}

	int res = GetIPVersion(pDest[0]);

	if(res == AF_INET) {
		result = mgr.Open();
	} else if(res == AF_INET6) {
#ifndef _USE_IPV4ONLY
		result = mgr.Open(IPPROTO_ICMPV6);
#endif
	}

	if(result>0) {
#ifndef _USE_IPV4ONLY
		mgr.SetVerbose(g_nVerbose);
#endif

		mgr.Ping(5);
		for(int i=0;i<mgr.m_nReqNum;i++) {
			if(res == AF_INET) {
				printf("%02d: %s %ldms, res:%d\n",
				i+1,inet_ntoa(mgr.m_sRespIp[i].ip),mgr.m_sRespIp[i].ts,
				mgr.m_nRespOk[i]);
			} else {
#ifndef _USE_IPV4ONLY
				inet_ntop(AF_INET6, (void*)&mgr.m_sRespIp6[i].ip, temp, sizeof(temp));
				printf("%02d: %s %ldms, res:%d\n",
					i+1,temp,mgr.m_sRespIp6[i].ts, mgr.m_nRespOk[i]);
#endif
			}
		}
		printf("Response [%d/%d]\n",mgr.m_nRespNum,mgr.m_nReqNum);
		
		mgr.Close();
	}
}

// Trace Test 용 IP
// 211.233.27.221, 125.144.16.42
int DoTrace(char *pDest)
{
    CIcmpMgr mgr;
	int	result;
	char tip[40];
	memset(tip,0,sizeof(tip));

	int res = GetIPVersion(pDest);

	if(res == AF_INET) {
		result = mgr.Open();
	} else if(res == AF_INET6) {
#ifndef _USE_IPV4ONLY
		result = mgr.Open(IPPROTO_ICMPV6);
#endif
	}

	if(result>0) {
#ifndef _USE_IPV4ONLY
		mgr.SetVerbose(g_nVerbose);
#endif
		printf("TRACE: %s\n",pDest);
		int num = mgr.Trace(pDest, 5, 30, 10);
		for(int i=0;i<num;i++) {
			if(res== AF_INET) {
				printf("%02d: %s %ld ms\n",
					i+1,inet_ntoa(mgr.m_sRespIp[i].ip),mgr.m_sRespIp[i].ts);
			} else {
#ifndef _USE_IPV4ONLY
				inet_ntop(AF_INET6, &mgr.m_sRespIp6[i].ip, tip, sizeof(tip));
				printf("%02d: %s %ld ms\n",
					i+1,tip,mgr.m_sRespIp6[i].ts);
#endif
			}
		}
		// close
		mgr.Close();
	}
	fprintf(stdout, "\n\n");
	return result;
}

int DoPingCmd(char *pDest)
{
    CIcmpMgr mgr;
	int result = 0;
	char szCmd[256], szProg[30];
	memset(szCmd,0,sizeof(szCmd));
	memset(szProg,0,sizeof(szProg));

	int res = GetIPVersion(pDest);
	if(res== AF_INET) {		
		strncpy(szProg, "ping", sizeof(szProg)-1);
	} else if(res== AF_INET6) {
		strncpy(szProg, "ping6", sizeof(szProg)-1);
	}

#ifdef _WIN32
	snprintf(szCmd, sizeof(szCmd)-1, "%s -n 5 -w 1 %s", szProg, pDest);
#else
	snprintf(szCmd, sizeof(szCmd)-1, "%s -c 5 -i 1 %s", szProg, pDest);
#endif

	FILE *fp;
	int state;
	char buff[MAXLINE];
	fp = popen(szCmd, "r");
	if (fp == NULL)
	{
		snprintf(szCmd, sizeof(szCmd)-1, "ping -n 5 -w 1 %s", pDest);
		fp = popen(szCmd, "r");
		if(fp == NULL) {
			perror("erro : ");
			exit(0);
		}
	}

	while(fgets(buff, MAXLINE, fp) != NULL)
	{
		printf("%s", buff);

		if(strstr(buff, "from") && strstr(buff, "time")) {
			result = 1;
			break;
		} else result = 0;
	}

	state = pclose(fp);
	//printf("state is %d\n", state);	

	return result;
}

int DoTraceCmd(char *pDest)
{
    CIcmpMgr mgr;
	int result = 0;
	char szCmd[256], szProg[30];
	memset(szCmd,0,sizeof(szCmd));
	memset(szProg,0,sizeof(szProg));

	int res = GetIPVersion(pDest);
	if(res== AF_INET) {
#ifdef _WIN32
		strncpy(szProg, "tracert", sizeof(szProg)-1);
#else
		strncpy(szProg, "traceroute", sizeof(szProg)-1);
#endif
	} else if(res== AF_INET6) {
#ifdef _WIN32
		strncpy(szProg, "tracert6", sizeof(szProg)-1);
#else
		strncpy(szProg, "traceroute6", sizeof(szProg)-1);
#endif
	}

#ifdef _WIN32
	snprintf(szCmd, sizeof(szCmd)-1, "%s -w 5 -h 30 %s", szProg, pDest);
#else
	snprintf(szCmd, sizeof(szCmd)-1, "%s -w 5 -m 30 %s", szProg, pDest);
#endif

	FILE *fp;
	int state;
	char buff[MAXLINE];
	fp = popen(szCmd, "r");
	if (fp == NULL)
	{
		snprintf(szCmd, sizeof(szCmd)-1, "ping -n 5 -w 1 %s", pDest);
		fp = popen(szCmd, "r");
		if(fp == NULL) {
			perror("erro : ");
			exit(0);
		}
	}

	while(fgets(buff, MAXLINE, fp) != NULL)
	{
		printf("%s", buff);

		if(strstr(buff, "from") && strstr(buff, "time")) {
			result = 1;
			break;
		} else result = 0;
	}

	state = pclose(fp);
	//printf("state is %d\n", state);	

	return result;
}

void DoTestPing(char *pDest)
{
	int result1, result2;

	fprintf(stdout, "############ ICMPMGR Ping #############\n");
	result1 = DoPing(pDest);

	fprintf(stdout, "\n\n############ Ping Command #############\n");
	result2 = DoPingCmd(pDest);

	if(result1 == result2) printf("\n<Ping Verification SUCCESS!!>\n\n");
	else printf("\n<Ping Verification FAIL!!>\n\n");
}

void DoTestMPing(char **pDest)
{
}

void DoTestTrace(char *pDest)
{
	int result1, result2;

	fprintf(stdout, "############ ICMPMGR Trace #############\n");
	result1 = DoTrace(pDest);

	fprintf(stdout, "\n\n############ Traceroute Command #############\n");
	result2 = DoTraceCmd(pDest);

	if(result1 == result2) printf("\n<Ping Verification SUCCESS!!>\n\n");
	else printf("\n<Ping Verification FAIL!!>\n\n");
}

// ping test fe80::221:70ff:fecd:6a0d(10.0.0.143, xp)
// fe80::859e:ae66:6bec:be55(10.0.0.44, vista)
// fe80::21a:64ff:fea1:4584(10.0.0.72, linux)
int main(int argc,char* argv[])
{
    if(argc < 2)
    {
        usage(argv[0]);
        exit(1);
    }

#ifdef _WIN32   // startup socket; needed ws2_32.lib
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 1, 1 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )  return 0;
#endif

   // Parse the command line
    int nOpt = ValidateArgs(argc, argv);
	
	switch(nOpt) {
	case OPTION_PING:
		if(g_pDest[1]) {
			DoMPing(g_pDest);
		} else {
			DoPing(g_pDest[0]);
		}
		break;

	case OPTION_TRACE:
		DoTrace(g_pDest[0]);
		break;

	case OPTION_TEST_PING:
		DoTestPing(g_pDest[0]);
		break;

	case OPTION_TEST_MULTIPING:
		DoTestMPing(g_pDest);
		break;

	case OPTION_TEST_TRACE:
		DoTestTrace(g_pDest[0]);
		break;

	case OPTION_TEST_FIND:
		if(g_pDest[1]) {
			DoTestFind(g_pDest[0], g_pDest[1]);
		}
		break;
	}

#ifdef _WIN32   // cleanup socket; needed ws2_32.lib
    WSACleanup();
#endif

	return 0;
}
#endif

#if defined(_ICMPMGR_TEST ) && !defined(_USE_IPV4ONLY)
int main(int argc,char* argv[])
{
    CMyIcmpMgr mgr;
	FILE* fp;
	char ip[256][32], temp[64];;
	int i,num;
	time_t	stime=time(NULL), etime=0;
	long ts;
	struct sockaddr_in from, dest;
	mysocklen_t fromlen = sizeof(from);

	sprintf(temp,"10.0.0.112"); mgr.Add(inet_network(temp));
	while(1) {
		// kskim
		if(mgr.Open()>0) {
			mgr.Ping(5);
			for(int i=0;i<mgr.m_nReqNum;i++) {
				printf("%02d: %s %dms, res:%d\n",
					i+1,inet_ntoa(mgr.m_sRespIp[i].ip),mgr.m_sRespIp[i].ts,
					mgr.m_nRespOk[i]);
			}
			printf("Response [%d/%d]\n",mgr.m_nRespNum,mgr.m_nReqNum);
			mgr.Close();
		}
		sleep(1);
	}
	return 0;

#ifdef _HJSON
	if(mgr.Open()>0) {
		mgr.Find("10.0.0.1","10.0.0.254",10);
		for(int i=0;i<mgr.m_nReqNum;i++) {
			if(mgr.m_nRespOk[i]) {
				printf("%02d: %s %d ms, res %d\n",
					i+1,inet_ntoa(mgr.m_sRespIp[i].ip),mgr.m_sRespIp[i].ts,
					mgr.m_nRespOk[i]);
			}
		}
		printf("Response [%d/%d]\n",mgr.m_nRespNum,mgr.m_nReqNum);
	}
	mgr.Close();
	return 0;

	if(mgr.Open() > 0) {
		printf("START...\n");
		//ts = mgr.Ping("72.14.235.104", 5);
		//ts = mgr.Ping("10.0.0.33", 5);
		ts = mgr.Ping("10.0.0.1", 5);
		etime = time(NULL);
		printf("END : %ld ms, time : %ld sec\n", ts, etime-stime);
	}
	mgr.Close();
	return 0;

	if(mgr.Open() > 0) {
		char tip[32];
		strcpy(tip,"211.233.27.221");
		strcpy(tip,"125.144.16.42");
		printf("TRACE: %s\n",tip);
		int num = mgr.Trace(tip);
		for(int i=0;i<num;i++) {
			printf("%02d: %s %d ms\n",
				i+1,inet_ntoa(mgr.m_sRespIp[i].ip),mgr.m_sRespIp[i].ts);
		}
		// close
		mgr.Close();
	}
#endif
	
	// open
	while(1) {
		if(mgr.Open() > 0) {
    		ts = mgr.Ping("125.159.3.72",5);
			printf("125.159.3.72 %d ms\n",ts);

    		ts = mgr.Ping("125.159.3.71",5);
			printf("125.159.3.71 %d ms\n\n",ts);

			// close
			mgr.Close();
		}
		sleep(1);
	}

	return 0;
}
#endif
