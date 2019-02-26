/*
 * Calculator : +, -, *, %, (, )
 *
 */

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <math.h>
#include <ctype.h>

#include "mycalc.h"

#ifndef myint64
#ifdef _WIN32
typedef __int64             myint64;
typedef unsigned __int64    umyint64;
#else
typedef long long           myint64;
typedef unsigned long long  umyint64;
#endif
#endif

CMyCalc::CMyCalc()
{
//#ifdef __SNMPMGR_H__
//	m_pSnmp = NULL;
//#endif
}

CMyCalc::~CMyCalc()
{
}

double CMyCalc::Calc(char *p, int len)
{
    int pos = 0, move;
    int neg = 0;

    while(p[pos] == '+' || p[pos] == '-') {
       	if(p[pos] == '-') neg = !neg;
       	pos++;
    }

    double d1 = GetNumber(p + pos, move);
    if(neg) d1 = -d1;
    pos += move;

    while(pos < len) {
       	char c = p[pos];
       	if(c == '+' || c == '-') return d1 + Calc(p+pos,len-pos);
       	if(c == '*' || c == '/' || c == '%' || c == '|' || c == '&') {
	    	pos++;
		    neg = 0;
		    while(p[pos] == '+' || p[pos] == '-')  {
	       		if(p[pos] == '-') neg =!neg;
		       	pos++;
		    }
		    double d2 = GetNumber(p + pos, move);
		    if(neg) d2 = -d2;
		    pos += move;
			//
			if(c == '|') d1 = (double)((myint64)d1 || (myint64)d2);
			else if(c == '&') d1 = (double)((myint64)d1 && (myint64)d2);
			else if(c == '*') d1 *= d2;
		    else if(d2 == 0) return 0; //throw "Division by Zero";
		    else if(c == '/') d1 /= d2;
		    else {
			//d1 = fmod(d1,d2);	// modified by hjson 2006.04.19.
			int times = 0;
			if(d2 != 0) times = (int)(d1/d2);
			d1 = (double)(d1 - (times*d2));
		    }
       	} else {
			fprintf(stderr,"Expected operator before %s",p + pos); 
			fprintf(stderr,"What ??? '%c'",p[pos]); 
		    return 0;
		}
    }
    return d1;
}

int CMyCalc::Funk(char *p, int binary, double &x, double &y) 
{
    int size = 0, nr=1;

    while( nr ) {
       	if(!p[size] ) return 0; //throw "Error: Missing ')'";
       	if( p[size] == '(') nr++;
       	if( p[size] == ')') nr--;
       	size++;
    }
    size--; // size är  antal tecken mellan () tex: (aaa) -> size=3

    if(binary) {
		int i;
       	for(i=0; i<size; i++)
       	{
	    	if(p[i] == '(') { 
				int nr = 1; 
				while(nr && i < size) {
		    		i++;
		    		if(p[i] == '(') nr++;
		    		if(p[i] == ')') nr--;
	       		}
	    	} 
	    	if(p[i] == ',') break;
       	}
       	if(i == size) return size + 1;
       	x = Calc(p, i);
       	y = Calc(p + i + 1, size - i - 1);
    } else
		x = Calc(p, size);

    return size + 1;
}

double CMyCalc::Factorial(double x)
{
    if(x < 0) return 0;
    double d = 1;
    for(int i=1; i<=(int)x; i++) d *= i;
    return d;
}

#define FX(FUNK,LEN,BIN,DO) if(!memcmp(p,FUNK,LEN)) { move = LEN + Funk(p+LEN,BIN,x,y); { DO; } }

double CMyCalc::GetNumber(char *p, int &move)
{
    double x,y;
    switch (tolower(*p)) {
	/*
    case 'a':
       	FX("abs("  ,4,0, return fabs(x););
       	FX("asin(" ,5,0, if(gra) return asin(x)*360/(2*PI); return asin(x););
       	FX("acos(" ,5,0, if(gra) return acos(x)*360/(2*PI); return acos(x););
       	FX("atan(" ,5,0, if(gra) return atan(x)*360/(2*PI); return atan(x););
       	break;
    case 'c':
       	FX("cos("  ,4,0, if(gra) x = 2*PI/360*x; return cos(x););
       	FX("cosh(" ,5,0, if(gra) x = 2*PI/360*x; return cosh(x););
       	FX("cpx("  ,4,1, double t=variabler["x"]; if(t>=x && t<y) return 1; return 0; );
       	break;
    case 'e':
       	FX("exp("  ,4,0, return exp(x););
       	break;
    case 'i':
       	FX("im("   ,3,0, return x.imag(););
       	FX("int("  ,4,0, if(x<0) return ceil(x); return floor(x););
       	break;
    case 'l':
       	FX("ln("   ,3,0, return log(x););
       	FX("log("  ,4,0, return log10(x););
       	break;
    case 'm':
       	FX("max("  ,4,1, if(x<y) return x; return y;);
       	FX("min("  ,4,1, if(x>y) return x; return y;);
       	break;
    case 'n':
       	FX("norm(" ,5,0, return norm(x););
       	break;
    case 'p':
       	FX("pow("  ,4,1, return pow(x,y););
       	FX("pol("  ,4,1, if(gra) y = 2*PI/360*y; return polar((double)x,(double)y););
       	break;
    case 'r':
       	FX("re("   ,3,0, return x.real(););
       	FX("rnd("  ,4,0, return double(x.real()*random()/0xffffffff,x.imag()*random()/0xffffffff); );
       	break;
    case 's':
       	FX("sqr("  ,4,0, return sqrt(x););
       	FX("sin("  ,4,0, if(gra) x = 2*PI/360*x; return sin(x););
       	FX("sqrn(" ,5,1, return pow(x,1/y);); // Bug på negativa x värden.
      	FX("sinh(" ,5,0, if(gra) x = 2*PI/360*x; return sinh(x););
       	break;
    case 't':
       	FX("tan("  ,4,0, if(gra) x = 2*PI/360*x; return tan(x););
       	FX("tanh(" ,5,0, if(gra) x = 2*PI/360*x; return tanh(x););
       	break;
    case '!':
       	FX("!("    ,2,0, return Factorial(x););
       	break;
	*/
    case '(':
       	FX("("     ,1,0, return x;)
	    break;
    }

    if(p[0] == '0' && (p[1] == 'x' || p[1] == 'X') ) {
       	int i=0; char c; move = 1;
       	sscanf(p,"%x",&i);
       	do { move++; c = tolower(p[move]); }
       	while( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')); 
		return (double)i;
    }

    if(p[0] == '0' && (p[1] == 'b' || p[1] == 'B') ) {
       	int i=0; move = 2;
       	while(p[move] == '0' || p[move] == '1') {
			i *= 2;
			if(p[move] == '1') i++;
			move++;
		}
       	return (double)i;
    }

    if( (*p>='0' && *p<='9') || *p=='.' ) {
       	double d = atof(p);
       	move = 0;
       	if   (p[move] == '+' || p[move] == '-') move++;
       	while(p[move] >= '0' && p[move] <= '9') move++;
       	if   (p[move] == '.') move++;
       	while(p[move] >= '0' && p[move] <= '9') move++;
       	if   (p[move] == '.')	{	// MIB
	    	move++;
	    	while(p[move] >= '0' && p[move] <= '9' || p[move] == '.') move++;

	    	unsigned int val = 0;
//#ifdef __SNMPMGR_H__
//	    	if(m_pSnmp) {
//	       		char buf[256];
//	       		memcpy(buf,p,move); buf[move] = '\0';
//				m_pSnmp->Get(buf,&val,sizeof(val));
//	    	}
//#endif
	    	d = (double)val;
		} else if(p[move] == 'e' || p[move] == 'E') {
	    	move++; 
	    	if   (p[move] == '+' || p[move] == '-') move++;
	    	while(p[move] >= '0' && p[move] <= '9') move++;
       	}
       	if(p[move] == 'i') { move++; return d; }
       	return d;
    }
   
    move = 0;
    /*
    int found = 0; move = 0;
    for(it=variabler.begin(); it!=variabler.end(); it++)
    {
       	if(memcmp( p, it->first.c_str(), it->first.size()) == 0)
       	{
	    found = 1;
	    if(it->first.size() > move) {
	        move = it->first.size();
		x = it->second;
	    }
       	}
    }
    if(found) return x;
    */

    if(memcmp(p,"pi",2)==0 && TestDelimiter(p[2]) ) {
		move = 2;
		return PI;
    }
    if(p[0]=='e' && TestDelimiter(p[1])) {
		move = 1;
		return 2.71828182845904523536;
    }  

    int i=0;
    char tmp[100];
    while( !TestDelimiter(p[i]) ) { tmp[i] = p[i]; i++; }
    tmp[i] = 0;
    
    return 0;
}

double CMyCalc::Calc(char *psz)
{
    int i=0,k=0;
    char p[5000];

    while(psz[k]) { 
		if(!isspace(psz[k])) { 
			p[i] = psz[k]; i++; 
		} 
		k++; 
	}
    if(!i) return 0;

    p[i] = 0;
    for(k=0; k<i; k++) if(p[k] == '=') break;
    if(k>=i) k=-1;
   
    newVar = 0; isVar = 0;
    double d = Calc(p+k+1,i-k-1);
    /*
    if(k >= 0) {
       	if(!isalpha(p[0])) return 0; //throw "Variable must begin with alpha";
       	for(i=1; i<k; i++) if( !isalnum(p[i]) ) return 0; //throw "A non alphanumeric value in variable";
       	string s;
       	s.append(p,k); 
	if(s=="e" || s=="PI" || s=="pi" ) return 0; //throw "Error: Reserved name"; 
	double &d2 = variabler[s];
       	if(d2 != d) { d2 = d; newVar = 1; }
       	isVar = 1;
    }
    */
    return d;
}

///////////////////////////////////////////////////////////
#ifdef _MYCALC_TEST
int main()
{
    CMyCalc calc;
    //double d = calc.Calc("100*(1 + 2 + 3)/50+22 - 50 ");
    double d = calc.Calc("1& 1");
    printf("d = %g\n",d);
    return 1;
}
#endif
