
#ifndef _AQUEUE_TEST
#include "stdafx.h"
#endif

#ifdef _WIN32
#pragma warning (disable : 4290)
#pragma warning (disable : 4244)
#pragma warning (disable : 4251)
#pragma warning (disable : 4275)
#pragma warning (disable : 4661)
#pragma warning (disable : 4786)    // map stl error disable
#pragma warning (disable : 4503)    // map stl error disable
#pragma warning (disable : 4305)    // truncation from 'const double' to 'float'
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "aqueue.h"
// AQueue 정상 작동 여부 검증
int AQueueTest()
{
    char* fail = NULL;
	AQueue<int> qInt;
	int *pi, nNum = 0;
	qInt.SetMax(13);

	qInt.Begin();
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	qInt.End();

    printf("AQueue Test 1: ");
	qInt.SetLast();
	while((pi=qInt.GetNext())) {
        if(*pi < 1 || *pi > 3) fail = "T1";
		printf("%d ",*pi);
	}
    if(fail == NULL) printf("[OK]\n");
    else printf("[FAIL]\n"); 

	qInt.Begin();
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	qInt.End();

	qInt.Begin();
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	qInt.End();

	qInt.Begin();
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	qInt.End();

    printf("AQueue Test 2: ");
	qInt.SetLast();
	while((pi=qInt.GetNext())) {
        if(*pi < 10 || *pi > 12) fail = "T2";
		printf("%d ",*pi);
	}
    if(fail == NULL) printf("[OK]\n");
    else printf("[FAIL]\n"); 

    printf("AQueue Test 3: no data ");
	qInt.SetLast();
	while((pi=qInt.GetNext())) {
        fail = "T3";
		printf("%d ",*pi);
	}
    if(fail == NULL) printf("[OK]\n");
    else printf("[FAIL]\n"); 

	qInt.Begin();
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	qInt.End();

	qInt.Begin();
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	qInt.End();

    printf("AQueue Test 4: ");
	while((pi=qInt.GetNext())) {
        if(*pi < 13 || *pi > 18) fail = "T4";
		printf("%d ",*pi);
	}
    if(fail == NULL) printf("[OK]\n");
    else printf("[FAIL]\n"); 

	qInt.Begin();
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	qInt.End();

    printf("AQueue Test 5: ");
	while((pi=qInt.GetNext())) {
        if(*pi < 19 || *pi > 21) fail = "T5";
		printf("%d ",*pi);
	}
    if(fail == NULL) printf("[OK]\n");
    else printf("[FAIL]\n"); 

	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);
	nNum++; qInt.AddTail(nNum);

    printf("AQueue Test 6: ");
	while((pi=qInt.GetNext())) {
        if(*pi < 22 || *pi > 24) fail = "T6";
		printf("%d ",*pi);
	}
    if(fail == NULL) printf("[OK]\n");
    else printf("[FAIL]\n"); 

	nNum++; qInt.AddTail(nNum);
    pi = qInt.GetNext();
    pi = qInt.GetNext();
    printf("AQueue Test 7: ");
    if(pi) fail = "T7";
    if(fail == NULL) printf("[OK]\n");
    else printf("[FAIL]\n"); 

    if(fail == NULL) printf("AQueue Result [OK]\n");
    else printf("AQueue Result [===Fail (%s)===]\n",fail);
    return (fail == NULL);
}

#ifdef _AQUEUE_TEST
int main(int argc,char* argv[])
{
	int aqokay = 0, aqtotal = 0;
	int num = 1;
	char* pitem = (char*)"all";

	if(argc >= 2) pitem = argv[1];
	if(argc >= 3) num = atoi(argv[2]);

	while(num-- > 0) {
		if(!strcmp(pitem,"all") || !strcmp(pitem,"aqueue")) {
			aqokay += AQueueTest();
			aqtotal++;
		}
	}
	if(!strcmp(pitem,"all") || !strcmp(pitem,"aqueue")) {
		printf("Tested AQueue [%s] total [%d] okay [%d]\n",
			aqtotal==aqokay ? "OK": "Fail",
			aqtotal,aqokay);
	}

	return 0;
}
#endif
