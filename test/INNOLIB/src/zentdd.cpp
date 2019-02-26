/////////////////////////////////////////////////////////////////////
// zen library 함수에 대한 검증 

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mutext.h"
#include "condext.h"

#include "file64.h"
#include "fold64.h"
#include "stat64.h"

#include "thread.h"
#include "mycalc.h"

#include "icmpmgr.h"

#include "svcapp.h"
#include "ihuman.h"
#include "killall.h"
#include "snprintf.h"

char ZenTddFailReason[256];

int TestCMutext()
{
	char* fail = NULL;
	CMutext mut;

	mut.Lock();
	fail = NULL;
	mut.Unlock();

	printf("Tested CMutext [OK]\n");
	return (fail == NULL);
}

int TestCCondext()
{
	char* fail = NULL;
	CCondext cond;

	cond.Lock();
	fail = NULL;
	cond.Unlock();

	printf("Tested CCondext [OK]\n");
	return (fail == NULL);
}

int TestCFile64()
{
	char* fail = NULL;
	CFile64 file;
	char *txt = (char*)"abcd 1234", buf[1024];

	memset(buf,0,sizeof(buf));
	// write
	if(file.Open((char*)"zentdd.txt",(char*)"wb")) {
		file.Write(txt,strlen(txt));
		file.Close();
	}
	// read
	if(file.Open((char*)"zentdd.txt",(char*)"rb")) {
		file.Read(buf,sizeof(buf)-1);
		file.Close();
	}
	if(strcmp(txt,buf)) fail = (char*)"mismatch read,write bytes";

	if(fail == NULL) printf("Tested CFile64 [OK]\n");
    else printf("Tested CFile64 [===Fail (%s)===] [%s]->[%s]\n",fail,txt,buf);
	return (fail == NULL);
}

int TestCFold64()
{
	char *fail = NULL;
	CFold64 fold;
	char buf[512];
	int nCount = 0;
	
	if(fold.Open((char*)".")) {
		while(fold.Read(buf,sizeof(buf)-1)) {
			nCount++;
		}
		fold.Close();
	}
	if(nCount <= 0) fail = (char*)"can't read folder";

	if(fail == NULL) printf("Tested CFold64 [OK]\n");
    else printf("Tested CFold64 [===Fail (%s)===]\n",fail);
	return (fail == NULL);
}

int TestCStat64()
{
	CStat64 st;
	char* fail = NULL;

	if(st.Stat((char*)".") == 0) {
		if(st.GetMTime() <= 0) fail = (char*)"failed to GetMTime()";
	} else {
		fail = (char*)"failed to Stat()";
	}

	if(fail == NULL) printf("Tested CStat64 [OK]\n");
    else printf("Tested CStat64 [===Fail (%s)===]\n",fail);
	return (fail == NULL);
}

///////////////////////////////////////////////////////////
// thread.h.cpp

static THREADTYPE _MyThread(LPVOID pParam)
{
	return 0;
}

int TestCThread()
{
	char* fail = NULL;
	CThread th;

	th.Create(_MyThread,NULL);
	th.Join();

	if(fail == NULL) printf("Tested CThread [OK]\n");
	else printf("Failed CThread [%s]\n",fail);
	return (fail == NULL);
}

int TestCMyCalc()
{
	char* fail = NULL;
	CMyCalc calc;
	double val;

	// Test 1
	val = calc.Calc((char*)"1+2+3+4+5");
	if(val != 15) fail = (char*)"mismatch sum";
	// Test 2
	val = calc.Calc((char*)"1+2*3+4-5");
	if(val != 6) fail = (char*)"mismatch multiply";

	if(fail == NULL) printf("Tested CMyCalc [OK]\n");
	else printf("Failed CMyCalc [%s]\n",fail);
	return (fail == NULL);
}

///////////////////////////////////////////////////////////
// icmpmgr.h.cpp

static int _TestIcmpGet(char* pIp,int nTimeout)
{
	if(strlen(pIp) == 0) return 0;

	CIcmpMgr icmp;
	int ret = 0;
	long ts;

	if(nTimeout < 5) nTimeout = 5;
	if(icmp.Open() > 0) {
		ts = icmp.Ping(pIp,nTimeout);
		if(ts >= 0) ret = 1;
		icmp.Close();
	}

	return ret;
}

int TestCIcmpMgr()
{
	char* fail = NULL;
	int ret, elap, timeout = 3;
	char ip[64];
	time_t start, end;

	start = time(NULL);
	strcpy(ip,"127.0.0.1");
	ret = _TestIcmpGet(ip,timeout);
	end = time(NULL);

	elap = end - start;
	if(ret != 1) fail = (char*)"failed to ping";
	else if(elap >= 7) fail = (char*)"ping time over";
	printf("CIcmpMgr: ip [%s] timeout [%d] elap [%d] ret [%d]\n",
		ip,timeout,elap, ret);

	start = time(NULL);
	strcpy(ip,"123.123.123.123");
	ret = _TestIcmpGet(ip,timeout);
	end = time(NULL);

	elap = end - start;
	if(ret != 0) fail = (char*)"ping 123.123.123.123";
	else if(elap >= 7) fail = (char*)"un-ping time over";
	printf("CIcmpMgr: ip [%s] timeout [%d] elap [%d] ret [%d]\n",
		ip,timeout,elap, ret);

	if(fail == NULL) printf("Tested CIcmpMgr [OK]\n");
    else printf("Tested CIcmpMgr [===Fail (%s)===]\n",fail);
	return (fail == NULL);
}

///////////////////////////////////////////////////////////
// ihuamn.cpp

int TestIHuman()
{
	char* fail = NULL;
	float val;
	char sval[32];

	// Test 1
	val = 100000000; 	// 100M
	IHumanReadable(val,sval,sizeof(sval),1000);
	if(strcmp(sval,(char*)"100M")) fail = (char*)"mismatch 100M";

	// Test 2
	val = 10000000; 	// 10M
	IHumanReadable(val,sval,sizeof(sval),1000);
	if(strcmp(sval,(char*)"10M")) fail = (char*)"mismatch 10M";

	// Test 3
	val = 100000; 		// 100K
	IHumanReadable(val,sval,sizeof(sval),1000);
	if(strcmp(sval,(char*)"100K")) fail = (char*)"mismatch 100K";

	// Test 4
	val = -10000000; 	// -10M
	IHumanReadable(val,sval,sizeof(sval),1000);
	if(strcmp(sval,(char*)"-10M")) fail = (char*)"mismatch -10M";

	if(fail == NULL) printf("Tested IHuman [OK]\n");
	else printf("Failed IHuman [%s]\n",fail);
	return (fail == NULL);
}

///////////////////////////////////////////////////////////
// killall.cpp

int Testkillall()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////

int TestZen()
{
	char* fail = NULL;
	int okay = 0;

	okay = TestCMutext();
	if(!okay) fail = (char*)"failed to test CMutext";

	okay = TestCCondext();
	if(!okay) fail = (char*)"failed to test CCondext";

	okay = TestCFile64();
	if(!okay) fail = (char*)"failed to test CFile64";

	okay = TestCFold64();
	if(!okay) fail = (char*)"failed to test CFold64";

	okay = TestCStat64();
	if(!okay) fail = (char*)"failed to test CStat64";
	
	okay = TestCMyCalc();
	if(!okay) fail = (char*)"failed to test CMyCalc";

	okay = TestCThread();
	if(!okay) fail = (char*)"failed to test CThread";

	okay = TestCIcmpMgr();
	if(!okay) fail = (char*)"failed to test CIcmpMgr";

	okay = TestIHuman();
	if(!okay) fail = (char*)"failed to test IHumanReadable";

	if(fail == NULL) printf("Tested ZEN [OK]\n");
	else printf("Failed ZEN [%s]\n",fail);

	return (fail == NULL);
}

int ZenTdd(char* pitem,int num)
{
	int zenokay = 0, zentotal = 0;

	while(num-- > 0) {
		if(!strcmp(pitem,"all") || !strcmp(pitem,"zen")) {
			zenokay += TestZen();
			zentotal++;
		}
	}
	if(zenokay != zentotal) strcpy(ZenTddFailReason,"Innogrid Library");
	if(!strcmp(pitem,"all") || !strcmp(pitem,"zen")) {
		printf("Tested ZEN [%s] total [%d] okay [%d]\n",
			zentotal==zenokay ? "OK" : "Fail",
			zentotal,zenokay);
	}

	return (zentotal != zenokay);
}
