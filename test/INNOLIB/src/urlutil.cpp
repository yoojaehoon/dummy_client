
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "urlutil.h"

int UrlGetToken(char* pData,int& nLen,char* pTok,int nTok,int& bHead)
{
    if(nLen == 0) return 0;

    int i, num, okay = 0;
    num = 0;
    for(i=0;i<nLen;i++) {
		if(num >= nTok-1) break;
        pTok[num++] = pData[i];
		if(pData[i] == '\n') {
			okay = 1;
			if(num <= 2) bHead = 0;
			break;
		}
    }
    pTok[num] = '\0';

    // move last string to first
    if(okay || num+1>=nTok) {
		nLen = nLen - num;
		memcpy(pData,&pData[num],nLen);
        pData[nLen] = '\0';
    } else {
        pTok[0] = '\0';
		num = 0;
    }

    return num;
}

void UrlGetHostPort(char* pUrl,char* pHost,int& nPort,char* pUri)
{
	int i, j, flag;
	char buf[512]; // 2008.12.14 jhson 64 -> 512

	char* pidx = strstr(pUrl,"://");
	if(pidx == NULL) return;

	nPort = 80;
	i = 0; j = 0; flag = 0;
	pidx += 3;
	while(*pidx != '/') {
		if(*pidx == ':') flag = 1;
		else if(flag == 1) {
			if(strlen(pidx) > 0) buf[j++] = *pidx;
		} else {
			if(strlen(pidx) > 0) pHost[i++] = *pidx;
		}
		pidx++;
	}
	buf[j] = '\0';
	pHost[i] = '\0';
	nPort = atoi(buf);
	if(nPort <= 0) nPort = 80;
	strcpy(pUri,pidx);
}

void UrlEncode(char* msg,int msgsize,char* encstr,int encsize)
{
    int i, j;
    unsigned char c;
    char buf[8];

    j = 0;
    for(i=0;i<msgsize;i++) {
        if(j >= encsize - 5) break;
        c = (unsigned char)msg[i];
		if(c == ' ') encstr[j++] = '+';
		else if(c >= '0' && c <= '9') encstr[j++] = c;
		else if(c >= 'A' && c <= 'Z') encstr[j++] = c;
		else if(c >= 'a' && c <= 'z') encstr[j++] = c;
		else if(c >= '.' && c <= '@') encstr[j++] = c;
		else {
            sprintf(buf,"%02x",c);
		    encstr[j++] = '%';
		    encstr[j++] = buf[0];
		    encstr[j++] = buf[1];
		}
    }
    encstr[j] = '\0';
}

static char _x2c(char hex_up,char hex_low)
{
	char digit;

	digit = 16 * (hex_up>='A' ? ((hex_up & 0xdf)-'A') + 10 : (hex_up - '0'));
	digit += (hex_low>='A' ? ((hex_low & 0xdf) - 'A') + 10 : (hex_low - '0'));

	return (digit);
}

void UrlDecode(char* str,int size)
{
	int i, j;

	for(i=j=0;j<size;i++,j++){
		switch(str[j]){
      	case '+':
			str[i] = ' ';
			break;
		case '%':
			str[i] = _x2c(str[j+1],str[j+2]);
			j += 2;
			break;
		default:
			str[i] = str[j];
			break;
		}
	}
	str[i]='\0';
}

#ifdef _URLUTIL_TEST

int main()
{
	char buf[256], enc[256];
	strcpy(buf,"event=2000&good=한글테스트");
	UrlEncode(buf,strlen(buf),enc,sizeof(enc));
	printf("URL %s\n",enc);
	//UrlDecode(buf,strlen(buf));
	//printf("URL %s\n",buf);
	return 0;
}

#endif
