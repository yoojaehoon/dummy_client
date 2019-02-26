/*
   Copyright (c) 2002-2005 BrainzSquare, Inc.
   ihuman.cpp - ihuman
 */

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <math.h>

#include "ihuman.h"

#ifdef _KSKIM
#include <math.h>
#endif

#ifdef WIN32
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#define MAX_SUFFIXES	9

static const char *suffixes[] =
{
	"B",	/* Byte 10^0 */
	"K",	/* Kilo 10^3 */
	"M",	/* Mega 10^6 */
	"G",	/* Giga 10^9 */
	"T",	/* Tera 10^12 */
	"P",	/* Peta 10^15 */
	"E",	/* Exa  10^18 */
	"Z",	/* Zetta 10^21 */
	"Y"	/* Yotta 10^24 */
};

char* IHumanReadable(myint64 val,char *buf,int len,int nHuman, int cipher)
{
	double dval = (double)val;
	return IHumanReadable(dval,buf,len,nHuman,cipher);
}

char* IHumanReadable(int val,char *buf,int len,int nHuman, int cipher)
{
	double dval = (double)val;
	return IHumanReadable(dval,buf,len,nHuman,cipher);
}

char* IHumanReadable(unsigned int val,char *buf,int len,int nHuman, int cipher)
{
	double dval = (double)val;
	return IHumanReadable(dval,buf,len,nHuman,cipher);
}

char* IHumanReadable(long val,char *buf,int len,int nHuman, int cipher)
{
	double dval = (double)val;
	return IHumanReadable(dval,buf,len,nHuman,cipher);
}

char* IHumanReadable(unsigned long val,char *buf,int len,int nHuman, int cipher)
{
	double dval = (double)val;
	return IHumanReadable(dval,buf,len,nHuman,cipher);
}

////////////////////////////////////////////////////////////////

void IHumanToDigit(char* buf,myint64& val,int nHuman)
{
    int i, num, len = strlen(buf);
    char tmp[32];

    num = 0;
    memset(tmp,0,sizeof(tmp));
    for(i=0;i<len;i++) {
	if(buf[i]>='0' && buf[i]<='9' || buf[i]=='.') tmp[i] = buf[i];
	else if(buf[i]=='B') num = 0;
	else if(buf[i]=='K') num = 1; 
	else if(buf[i]=='M') num = 2; 
	else if(buf[i]=='G') num = 3; 
	else if(buf[i]=='T') num = 4; 
	else if(buf[i]=='P') num = 5; 
	else if(buf[i]=='E') num = 6; 
	else break;
    }

    val = 1;
    for(i=0;i<num;i++) val *= nHuman;
    val = (myint64)(val*atof(tmp));
}

void IHumanToDigit(char* buf,unsigned int& val,int nHuman)
{
    myint64 val64 = 0;
    IHumanToDigit(buf,val64,nHuman);
    val = (unsigned int)val64;
}

// modified by moon 2006.2.28
// modified by kskim 2006.3.1 (ex. 123.1 -> 123.09)
// modified by kskim 2006.3.2 (ex. 123.0000 -> 123.0000)
// Hmmm... when using Zenius's snprintf, not available '%.*f'
// modified by kskim 2006.11.6 (ex. 123.234 -> 123)
// added cipher by kskim 2007.10.25
// modified by kskim 2007.10.26 (ex. -123.2345 -> -123.23)
char* IHumanReadable(double val, char *buf, int len, int nHuman, int cipher)
{
	double q;
	double r = 0;
	int    n = -1, minus = 0;
	char    sztmp[32], szDeci[32], szSuffix;

	if(val < 0) {
		val *= -1; minus = 1;
	}

	if (!buf)
		return NULL;

	// 양수만 고려한 루틴 
	memset(buf, 0, len);
	if (nHuman && val >= nHuman) {
		n = 0;
		for ( ; ; ) {
			q = (double)(val / (float) nHuman);
			if (q >= 1) {
				r = (double)(val - q * nHuman);
				val = q;
				n++;
			} else {
				break;
			}
		}
		if (n >= MAX_SUFFIXES) {
			buf[0] = '\0';
			return buf;
		}
	}

	if(cipher <= 0 ) cipher = 0;

	memset(sztmp, 0, sizeof(sztmp));
	memset(szDeci,0,sizeof(szDeci));

	mysnprintf(sztmp,sizeof(sztmp),"%lf", (double) (val - (int)val));

	if(cipher > 0) {
		strncpy(szDeci ,sztmp+2, (cipher>32) ? 32 : cipher);

		for(int i= strlen(szDeci) -1; i>=0; i--) {
			if(szDeci[i] == '0') szDeci[i] = '\0';
			else break;
		}
	}

	if(n < 0)  szSuffix = '\0';
	else szSuffix = suffixes[n][0];

	memset(buf,0,sizeof(buf));
	if(strlen(szDeci) > 0) {
		if(minus) mysnprintf(buf,len-1,"-%d.%s%c",(int) val,szDeci, szSuffix);
		else mysnprintf(buf,len-1,"%d.%s%c",(int) val,szDeci, szSuffix);
	} else {
		if(minus) mysnprintf(buf,len-1,"-%d%c",(int) val, szSuffix);
		else mysnprintf(buf,len-1,"%d%c",(int) val, szSuffix);
	}

	return buf;
}

char* IHumanReadable(float val,char *buf,int len,int nHuman, int cipher)
{
	double dval = (double) val;
	return IHumanReadable(dval,buf,len,nHuman, cipher);
}

////////////////////////////////////////////////////////////////

char* IHumanSecToDate(time_t ttime,char* buf,int len)
{
	memset(buf,0,len);
	if(ttime <= 0) return buf;

	struct tm lt;
	memcpy(&lt,localtime(&ttime),sizeof(lt));

#ifdef _WIN32
	_snprintf(buf,len,"%02d/%02d/%04d %02d:%02d:%02d",
			lt.tm_mon+1,lt.tm_mday,lt.tm_year+1900,
			lt.tm_hour,lt.tm_min,lt.tm_sec);
#else
	mysnprintf(buf,len,"%02d/%02d/%04d %02d:%02d:%02d",
			lt.tm_mon+1,lt.tm_mday,lt.tm_year+1900,
			lt.tm_hour,lt.tm_min,lt.tm_sec);
#endif

	return buf;
}

char* IHumanSecToTime(time_t ttime,char* buf,int len)
{
	// day
	time_t nDay = (time_t)(ttime / 86400.0);
		
	// hour
	ttime = ttime - nDay * 86400;
	time_t nHour = (time_t)(ttime / 3600.0);

	// min
	ttime = ttime - nHour * 3600;
	time_t nMin = (time_t)(ttime / 60.0);

	// sec
	time_t nSec = ttime - nMin * 60;

#ifdef _WIN32
	if(nDay > 0) 
		_snprintf(buf,len,"%ld days, %02ld:%02ld:%02ld",nDay,nHour,nMin,nSec);
	else
		_snprintf(buf,len,"%02ld:%02ld:%02ld",nHour,nMin,nSec);
#else
	if(nDay > 0) 
		mysnprintf(buf,len,"%ld days, %02ld:%02ld:%02ld",nDay,nHour,nMin,nSec);
	else
		mysnprintf(buf,len,"%02ld:%02ld:%02ld",nHour,nMin,nSec);
#endif

	return buf;
}

#if     (_HPUX || _OSF1)
myint64 strtoll(const char* str,char** ptr,int base)
{
	myint64 ll = 0;
	sscanf(str,"%lld",&ll);
	return ll;
}
#if _HPUX!=1123 && _HPUX!=1131
myint64 strtoull(const char* str,char** ptr,int base)
{
	myint64 ll = 0;
	sscanf(str,"%lld",&ll);
	return ll;
}
#endif
#endif

#ifdef _IHUMANCHK_TEST
int main()
{
	double a[5] = { 12.123, 1234123.12354, 12354123154.0, 123, 1000000000 };
	char sztmp[128], sztmp1[128];
	for(int i =0; i<5; i++) {
		printf("a[%d] = [%s], if 1024 then [%s]\n", i, 
				IHumanReadable(a[i], sztmp, sizeof(sztmp), 1000, 2),
				IHumanReadable(a[i], sztmp1, sizeof(sztmp1), 1024, 2));
	}
	printf("\n");
	double b[5] = { 21.12, -4100, -1234.12354, -12354123154.0, -123 };
	for(int i =0; i<5; i++) {
		printf("b[%d] = [%s], if 1024 then [%s]\n",i, 
				IHumanReadable(b[i], sztmp, sizeof(sztmp), IHUMAN_1000, 2),
				IHumanReadable(b[i], sztmp, sizeof(sztmp), IHUMAN_1024, 2));
	}
	
}
#endif
