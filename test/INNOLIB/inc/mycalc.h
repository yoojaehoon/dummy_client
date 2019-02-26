///////////////////////////////////////////////////
// mycalc.h - calculator
//
// 2005.07.01. added operator; | (or), & (and)

//#ifdef __SNMPMGR_H__
//#include "snmpmgr.h"
//#endif

#ifndef __MYCALC_H__
#define __MYCALC_H__

#define PI 3.1415926535897932384626433832795

//! 간단하게 구현된 사칙연산을 위한 클래스
class CMyCalc
{
    int Funk(char *p, int binary, double &x, double &y);
    double Factorial(double x);
    double GetNumber(char *p, int &move);
    int TestDelimiter(char c)
    {
       	return (c=='+' || c=='-' || c=='*' || c=='/'
			|| c=='%' || c==',' || c==')' || c==0
			|| c=='&' || c=='|' || c=='~');
    }

public:
    int gra,newVar,isVar;
//#ifdef __SNMPMGR_H__
//	CSnmpMgr* m_pSnmp;
//#endif

    CMyCalc();
    ~CMyCalc();

    double Calc(char *psz);
    double Calc(char *p, int len);
};

#endif
