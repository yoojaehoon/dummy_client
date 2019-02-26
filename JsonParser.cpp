#include "header.h"
#include "JsonParser.h"

#define BUFFER_BASE_SIZE 2048

CJsonParser::CJsonParser()
{
    m_strData = "";
    m_logflag = 0;
}

CJsonParser::~CJsonParser()
{

}

// Read Initialize
void CJsonParser::InitData(char* data, int logflag)
{
    m_strData = data;
    m_logflag = logflag;
}

int CJsonParser::ReadParse()
{
    int result = m_reader.parse(m_strData, m_root);
    
//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadParse", -1, 1, &result);

    return result;
}

/////////////////////////////////////////////////////////////////////
// Read
/////////////////////////////////////////////////////////////////////
// 단일 값 읽기
int CJsonParser::ReadValueInt(char *key, int &intvalue)
{
    if( !m_root.isMember(key) )
        return FALSE;

    intvalue = m_root.get(key, 0).asInt();

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadValueInt", -1, 1, &intvalue);

    return TRUE;    
}
int CJsonParser::ReadValueDouble(char *key, double &dbvalue)
{
    if( !m_root.isMember(key) )
        return FALSE;

    dbvalue = m_root.get(key, 0).asDouble();

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadValueDouble", -1, 2, &dbvalue);

    return TRUE;    
}
int CJsonParser::ReadValueBool(char *key, bool &boolvalue)
{
    if( !m_root.isMember(key) )
        return FALSE;

    boolvalue = m_root.get(key, 0).asBool();

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadValueDouble", -1, 1, &boolvalue);

    return TRUE;    
}
int CJsonParser::ReadValueString(char *key, char *strvalue)
{
    if( !m_root.isMember(key) )
        return FALSE;

    int size = strlen(m_root.get(key, "").asCString());
    memcpy( strvalue, m_root.get(key, "").asCString(), size);

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadValue", -1, 3, strvalue);

    return TRUE;    
}

// 배열 읽기
int CJsonParser::GetReadArrayCount(char *key)
{
    int result = 0;

    if( !m_root.isMember(key) )
        return result;

    const Json::Value valData = m_root[key];
    result = valData.size();

    return result;
}
int CJsonParser::ReadArray(char *key, int index, char *strvalue)
{
    if( !m_root.isMember(key) )
        return FALSE;

    const Json::Value valData = m_root[key];
    int count = valData.size();

    if( count <= index )
        return FALSE;

    int size = strlen(valData[index].asCString());
    memcpy( strvalue, valData[index].asCString(), size);

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadArray", index, 3, strvalue);

    return TRUE;    
}

// 다중 배열 값 읽기
int CJsonParser::ReadArrayValueString(char *key1, char *key2, char *strvalue)
{
    if( !m_root.isMember(key1) )
        return FALSE;

    int size = strlen(m_root[key1].get(key2, "").asCString());
    memcpy( strvalue, m_root[key1].get(key2, "").asCString(), size);

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadArrayValueString", -1, 3, strvalue);

    return TRUE;    
}
int CJsonParser::ReadArrayValueInt(char *key1, char *key2, int &intvalue)
{
    if( !m_root.isMember(key1) )
        return FALSE;

    intvalue = m_root[key1].get(key2, 0).asInt();

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadArrayValueString", -1, 1, &intvalue);

    return TRUE;    
}
int CJsonParser::ReadArrayValueDouble(char *key1, char *key2, double &dbvalue)
{
    if( !m_root.isMember(key1) )
        return FALSE;

    dbvalue = m_root[key1].get(key2, 0).asDouble();

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadArrayValueString", -1, 2, &dbvalue);

    return TRUE;    
}

int CJsonParser::ReadArrayValueBool(char *key1, char *key2, bool &boolvalue)
{
    if( !m_root.isMember(key1) )
        return FALSE;

    boolvalue = m_root[key1][key2].asBool();

//  if( m_logflag )
//      CMiscUtil::WriteLog("CJsonParser::ReadArrayValueString", -1, 1, &boolvalue);

    return TRUE;    
}

/////////////////////////////////////////////////////////////////////
// Write
/////////////////////////////////////////////////////////////////////
int CJsonParser::MakeWrite(char *strvalue)
{
    string m_strData = m_writer.write(m_root);

    sprintf(strvalue, "%s", m_strData.c_str());

    return TRUE;
}

int CJsonParser::WriteValueString(char *key, char *strvalue)
{
    m_root[key] = strvalue;
    
    return TRUE;
}
int CJsonParser::WriteValueInt(char *key, int value)
{
    m_root[key] = value;
    
    return TRUE;
}
int CJsonParser::WriteValueDouble(char *key, double value)
{
    m_root[key] = value;
    
    return TRUE;
}
int CJsonParser::WriteValueBool(char *key, bool value)
{
    m_root[key] = value;
    
    return TRUE;
}

int CJsonParser::WriteValueLongLong(char *key, long long value)
{
    m_root[key] = value;

    return TRUE;
}

// Key 하나에 중복 데이터
int CJsonParser::WriteArrayAppend(char *key, char *strvalue)
{
    Json::Value valData = m_root[key];

    valData.append(strvalue);

    m_root[key] = valData;

    return TRUE;
}

// Key 하나에 여러 쌍 데이터
int CJsonParser::WriteArrayValueString(char *key1, char *key2, char *strvalue)
{
    Json::Value valData;

    if( m_root.isMember(key1) )
        valData = m_root[key1];
    
    valData[key2] = strvalue;
    m_root[key1] = valData; 
    
    return TRUE;
}
int CJsonParser::WriteArrayValueInt(char *key1, char *key2, int value)
{
    Json::Value valData;

    if( m_root.isMember(key1) )
        valData = m_root[key1];
    
    valData[key2] = value;
    m_root[key1] = valData; 

    return TRUE;
}
int CJsonParser::WriteArrayValueDouble(char *key1, char *key2, double value)
{
    Json::Value valData;

    if( m_root.isMember(key1) )
        valData = m_root[key1];
    
    valData[key2] = value;
    m_root[key1] = valData; 

    return TRUE;
}
int CJsonParser::WriteArrayValueBool(char *key1, char *key2, bool value)
{
    Json::Value valData;

    if( m_root.isMember(key1) )
        valData = m_root[key1];
    
    valData[key2] = value;
    m_root[key1] = valData; 

    return TRUE;
}

/*int main(int argc, char *argv[])
{
    char buffer[1024];
    CJsonParser w_json;
    w_json.WriteValueString("name", "mynameisjson");
    w_json.MakeWrite(buffer);

    printf("json : %s", buffer);
}
*/
