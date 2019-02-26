#ifndef _JSONPARSER_H
#define _JSONPARSER_H

#include <string>
#include <json/json.h> 

using namespace std;

// 패킷 클래스는 받기와 보내기시 1회용으로만 사용되도록 설계 되었음
class CJsonParser
{
    public:
        CJsonParser();
        ~CJsonParser();

        void InitData(char* data, int logflag=0);
        int ReadParse();

        int ReadValueInt(char *key, int &intvalue);
        int ReadValueDouble(char *key, double &dbvalue);
        int ReadValueBool(char *key, bool &boolvalue);
        int ReadValueString(char *key, char *strvalue);

        int GetReadArrayCount(char *key);
        int ReadArray(char *key, int index, char *strvalue);

        int ReadArrayValueString(char *key1, char *key2, char *strvalue);
        int ReadArrayValueInt(char *key1, char *key2, int &intvalue);
        int ReadArrayValueDouble(char *key1, char *key2, double &dbvalue);
        int ReadArrayValueBool(char *key1, char *key2, bool &boolvalue);

        int MakeWrite(char *strvalue);
        int WriteValueString(char *key, char *strvalue);
        int WriteValueInt(char *key, int value);
        int WriteValueDouble(char *key, double value);
        int WriteValueBool(char *key, bool value);
        int WriteValueLongLong(char *key, long long value);

        int WriteArrayAppend(char *key, char *strvalue);

        int WriteArrayValueString(char *key1, char *key2, char *strvalue);
        int WriteArrayValueInt(char *key1, char *key2, int value);
        int WriteArrayValueDouble(char *key1, char *key2, double value);
        int WriteArrayValueBool(char *key1, char *key2, bool value);
    
    private:
        string m_strData;
        Json::Value m_root;
        Json::Reader m_reader;
        Json::StyledWriter m_writer;

        int m_logflag;  // 1:로그 남김, 0:로그 안남김
};

#endif // _JSONPARSER_H


