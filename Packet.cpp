#include "header.h"
#include "Packet.h"

#define BUFFER_BASE_SIZE 2048

CPacket::CPacket()
{
    m_nCategory = 0;
    m_nCommand = 0;
    m_nDataLength = 0;
    m_nDataPointer = 0;
    m_strData = NULL;
    m_logflag = 1;
}

CPacket::~CPacket()
{
    if (m_strData != NULL)
        delete [] m_strData;
}

void CPacket::InitPacket(int client_sockfd)
{
    m_nClientSockfd = client_sockfd;
}

void CPacket::InitPacket(int client_sockfd, int logflag)
{
    m_nClientSockfd = client_sockfd;
    m_logflag = logflag;   
}

void CPacket::InitPacket(int category, int command, int client_sockfd)
{
    m_nCategory = category;
    m_nCommand = command;
    m_nClientSockfd = client_sockfd;
}

void CPacket::InitPacket(int category, int command, int client_sockfd, int logflag)
{
    m_nCategory = category;
    m_nCommand = command;
    m_nClientSockfd = client_sockfd;

    m_logflag = logflag;
}

int CPacket::ReadTinyInt(int &intvalue)
{
    if (!CheckDataExists(1))
        return FALSE;
    
    char value;
    
    memcpy(&value, m_strData + m_nDataPointer, 1);

    intvalue = (int)value;
        
    m_nDataPointer += 1;
    
    //CMiscUtil::WriteLog("CPacket::ReadTinyInt", m_nClientSockfd, 1, &intvalue);
    
    return TRUE;
}

int CPacket::ReadShortInt(int &intvalue)
{
    if (!CheckDataExists(2))
        return FALSE;
    
    short value;
    
    memcpy(&value, m_strData + m_nDataPointer, 2);

    intvalue = (int)ntohs(value);
        
    m_nDataPointer += 2;
    
    //CMiscUtil::WriteLog("CPacket::ReadShortInt", m_nClientSockfd, 1, &intvalue);
    
    return TRUE;
}

int CPacket::ReadInt(int &intvalue)
{
    if (!CheckDataExists(4))
        return FALSE;
    
    memcpy(&intvalue, m_strData + m_nDataPointer, 4);
    intvalue = ntohl(intvalue);
    
    m_nDataPointer += 4;
    
    //CMiscUtil::WriteLog("CPacket::ReadInt", m_nClientSockfd, 1, &intvalue);
    
    return TRUE;
}

int CPacket::ReadBigInt(long long &longvalue)
{
    int lowpart, highpart;
    
    if (!ReadInt(lowpart))
        return FALSE;

    if (!ReadInt(highpart))
        return FALSE;

    memcpy(&longvalue, &highpart, 4);
    memcpy((char *)&longvalue + 4, &lowpart, 4);
        
    //CMiscUtil::WriteLog("CPacket::ReadBigInt", m_nClientSockfd, 2, &longvalue);
    
    return TRUE;
}

int CPacket::ReadChar(char *charvalue, int size)
{
    if (!CheckDataExists(size) || size < 0)
        return FALSE;
        
    memcpy(charvalue, m_strData + m_nDataPointer, size);
    *(charvalue + size) = 0;
    
    m_nDataPointer += size;
    
    //CMiscUtil::WriteLog("CPacket::ReadChar", m_nClientSockfd, 3, charvalue);
    
    return TRUE;    
}

int CPacket::ReadShortString(char *stringvalue, int maxsize)
{
    int size;
    
    if (!ReadShortInt(size))
        return FALSE;
        
    if (!CheckDataExists(size))
        return FALSE;
        
    if (size < 0 || size > maxsize)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReadShortString", m_nClientSockfd, 1, &size);        
        return FALSE;
    }
    
    memcpy(stringvalue, m_strData + m_nDataPointer, size);
    *(stringvalue + size) = 0;
    
    m_nDataPointer += size;
    
    //CMiscUtil::WriteLog("CPacket::ReadShortString", m_nClientSockfd, 3, stringvalue);
    
    return TRUE;
}

int CPacket::ReadString(char *stringvalue, int maxsize)
{
    int size;
    
    if (!ReadInt(size))
        return FALSE;
        
    if (!CheckDataExists(size))
        return FALSE;
        
    if (size < 0 || size > maxsize)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReadString", m_nClientSockfd, 1, &size);     
        return FALSE;
    }
    
    memcpy(stringvalue, m_strData + m_nDataPointer, size);
    *(stringvalue + size) = 0;
    
    m_nDataPointer += size;
    
    //CMiscUtil::WriteLog("CPacket::ReadString", m_nClientSockfd, 3, stringvalue);
    
    return TRUE;
}

int CPacket::WriteTinyInt(int intvalue)
{
    CheckDataBuffer(1);

    //CMiscUtil::WriteLog("CPacket::WriteTinyInt", m_nClientSockfd, 1, &intvalue);
    
    char value = (char)intvalue;
    memcpy(m_strData + m_nDataPointer, &value, 1);
    
    m_nDataPointer += 1;
    
    return TRUE;
}

int CPacket::WriteShortInt(int intvalue)
{
    CheckDataBuffer(2);

    //CMiscUtil::WriteLog("CPacket::WriteShortInt", m_nClientSockfd, 1, &intvalue);
    
    short value = intvalue;
    value = htons(value);
    memcpy(m_strData + m_nDataPointer, &value, 2);
    
    m_nDataPointer += 2;
    
    return TRUE;
}

int CPacket::WriteInt(int intvalue)
{
    CheckDataBuffer(4);

    //CMiscUtil::WriteLog("CPacket::WriteInt", m_nClientSockfd, 1, &intvalue);
    
    intvalue = htonl(intvalue);
    memcpy(m_strData + m_nDataPointer, &intvalue, 4);
    
    m_nDataPointer += 4;
    
    return TRUE;
}

int CPacket::WriteBigInt(long long longvalue)
{
    CheckDataBuffer(8);
    
    //CMiscUtil::WriteLog("CPacket::WriteBigInt", m_nClientSockfd, 2, &longvalue);
    
    int lowpart, highpart;
    
    memcpy(&lowpart, &longvalue, 4);
    memcpy(&highpart, (char *)&longvalue + 4, 4);
    
    highpart = htonl(highpart);
    memcpy(m_strData + m_nDataPointer, &highpart, 4);

    m_nDataPointer += 4;
    
    lowpart = htonl(lowpart);
    memcpy(m_strData + m_nDataPointer, &lowpart, 4);
    
    m_nDataPointer += 4;
    
    return TRUE;    
}

int CPacket::WriteChar(char *charvalue, int size)
{
    CheckDataBuffer(size);
    
    //CMiscUtil::WriteLog("CPacket::WriteChar", m_nClientSockfd, 3, charvalue);
    
    memcpy(m_strData + m_nDataPointer, charvalue, size);
    
    m_nDataPointer += size;
    
    return TRUE;
}

int CPacket::WriteString(char *stringvalue)
{
    int size = strlen(stringvalue);

    if (!WriteInt(size))
        return FALSE;

    //CMiscUtil::WriteLog("CPacket::WriteString", m_nClientSockfd, 3, stringvalue);
        
    CheckDataBuffer(size);
    
    memcpy(m_strData + m_nDataPointer, stringvalue, size);
    
    m_nDataPointer += size;
    
    return TRUE;
}

int CPacket::WriteShortString(char *stringvalue)
{
    int size = strlen(stringvalue);

    if (!WriteShortInt(size))
        return FALSE;

    //CMiscUtil::WriteLog("CPacket::WriteShortString", m_nClientSockfd, 3, stringvalue);
        
    CheckDataBuffer(size);
    
    memcpy(m_strData + m_nDataPointer, stringvalue, size);
    
    m_nDataPointer += size;
    
    return TRUE;
}

int CPacket::IsEOD()
{
    if (m_nDataLength == m_nDataPointer)
        return TRUE;
    else
        return FALSE;
}

int CPacket::ReceivePacket()
{
    char header[10];
    unsigned int packetlength;
    
    int nread = read(m_nClientSockfd, header, 10);

    if (nread != 10)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket", m_nClientSockfd, 3, "header read()");

        return FALSE;
    }
    
    if (header[0] != (char)0xFF || header[1] != (char)0xFF)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket", m_nClientSockfd, 3, "header 체크 오류");

        return FALSE;
    }
    
    if (header[2] != (char)PROTOCOL_VER)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket", m_nClientSockfd, 3, "version 체크 오류");

        return FALSE;
    }
    
    memcpy(&packetlength, header + 3, 4);
    packetlength = ntohl(packetlength);
    
    m_nCategory = (int)header[7];
    m_nCommand = (int)header[8];
    
    m_nDataLength = (int)packetlength - 10;
    
    if (m_nDataLength < 0)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket", m_nClientSockfd, 3, "length 체크 오류");

        return FALSE;
    }
        
    m_strData = new char[m_nDataLength];

    m_nDataPointer = 0;
    
    while (m_nDataPointer < m_nDataLength)
    {
        nread = read(m_nClientSockfd, m_strData + m_nDataPointer, m_nDataLength - m_nDataPointer);
        
        if (nread < 1)
        {
            CMiscUtil::WriteErrorLog("CPacket::ReceivePacket", m_nClientSockfd, 3, "read()");
            
            return FALSE;
        }
        
        m_nDataPointer += nread;
    }

    CMiscUtil::WriteLog("CPacket::ReceivePacket", m_nClientSockfd, 1, &m_nCommand);
            
    m_nDataPointer = 0;

    return TRUE;
}

int CPacket::ReceivePacket_Select()
{
    char header[10];
    unsigned int packetlength;
    int result;

    fd_set readfds, testfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(m_nClientSockfd, &readfds);

    testfds = readfds;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    result = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set *)NULL, &timeout);

    if( result == 0)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket_Select() - RECEIVE TIMEOUT", m_nClientSockfd, 0);
        return FALSE;
    }

    if( result == -1 )                                                                       
    {   
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket_Select() - SOCKET CLOSE", m_nClientSockfd, 0);
        return FALSE;
    }   
        
    int nread = read(m_nClientSockfd, header, 10);                                           
        
    if( nread != 10 )
    {                                                                                        
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket_Select()", m_nClientSockfd, 3, "header read()");                                                                                  
        return FALSE;                                                                        
    }                                                                                        
        
    if( header[0] != (char)0xFF || header[1] != (char)0xFF )                                 
    {   
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket_Select", m_nClientSockfd, 3, "header 체크 오류");
    }

    if( header[2] != (char)PROTOCOL_VER)
    {
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket_Select()", m_nClientSockfd, 3, "version 체크 오류");
        return FALSE;
    }

    // Packet Size
    memcpy(&packetlength, header + 3, 4);
    packetlength = ntohl(packetlength);

    m_nCategory = (int)header[7];
    m_nCommand = (int)header[8];

    m_nDataLength = (int)packetlength - 10;

    if( m_nDataLength < 0 )
    {
        CMiscUtil::WriteErrorLog("CPacket::ReceivePacket_Select()", m_nClientSockfd, 3, "length 체크 오류");
        return FALSE;
    }

    m_strData = new char[m_nDataLength];

    m_nDataPointer = 0;

    while( m_nDataPointer < m_nDataLength )
    {
        nread = read(m_nClientSockfd, m_strData + m_nDataPointer, m_nDataLength - m_nDataPointer);

        if( nread < 1 )
        {
            CMiscUtil::WriteErrorLog("CPacket::ReceivePacket_Select()", m_nClientSockfd, 3, "read()");
            return FALSE;
        }

        m_nDataPointer += nread;
    }

    if( m_logflag )
        CMiscUtil::WriteLog("CPacket::ReceivePacket_Select()", m_nClientSockfd, 1, &m_nCommand);

    m_nDataPointer = 0;

    return TRUE;
}

int CPacket::SendPacket()
{
    if (m_nCategory == 0 || m_nCommand == 0)
        return FALSE;
        
    char header[10];
    unsigned int packetlength;
    
    memset(header, 0, sizeof(header));
    
    header[0] = (char)0xFF;
    header[1] = (char)0xFF;
    header[2] = (char)PROTOCOL_VER;
    
    packetlength = (unsigned int)m_nDataPointer + 10;
    packetlength = htonl(packetlength);
    
    memcpy(header + 3, &packetlength, 4);
    
    header[7] = (char)m_nCategory;
    header[8] = (char)m_nCommand;
    header[9] = 0;
    
    int nsend = write(m_nClientSockfd, header, 10);

    if (nsend != 10)
    {
        CMiscUtil::WriteErrorLog("CPacket::SendPacket header length ", m_nClientSockfd, 1, &nsend);
        CMiscUtil::WriteErrorLog("CPacket::SendPacket", m_nClientSockfd, 3, "header write()");

        return FALSE;
    }
    
    int totalsend = 0;
    
    while (totalsend < m_nDataPointer)
    {
        if (m_nDataPointer - totalsend > PACKET_SIZE)
        {
            nsend = write(m_nClientSockfd, m_strData + totalsend, PACKET_SIZE);
            
            if (nsend < 1)
            {
                CMiscUtil::WriteErrorLog("CPacket::SendPacket", m_nClientSockfd, 3, "write()");
            
                return FALSE;
            }
        }
        else
        {
            nsend = write(m_nClientSockfd, m_strData + totalsend, m_nDataPointer - totalsend);
            
            if (nsend < 1)
            {
                CMiscUtil::WriteErrorLog("CPacket::SendPacket", m_nClientSockfd, 3, "write()");
            
                return FALSE;
            }
        }
        
        totalsend += nsend;
    }
    
    //CMiscUtil::WriteLog("CPacket::SendPacket", m_nClientSockfd, 1, &m_nCommand);
    
    return TRUE;
}

int CPacket::CheckDataExists(int size)
{
    if (m_nDataLength - m_nDataPointer >= size)
        return TRUE;
    else
    {
        CMiscUtil::WriteErrorLog("CPacket::CheckDataExists", m_nClientSockfd, 1, &size);

        return FALSE;
    }   
}

void CPacket::CheckDataBuffer(int size)
{
    while (m_nDataLength - m_nDataPointer < size)
    {
        char *olddata = m_strData;
        
        if (m_nDataLength == 0)
            m_nDataLength = BUFFER_BASE_SIZE;
        else
            m_nDataLength *= 2;
                
        m_strData = new char[m_nDataLength];
    
        memset(m_strData, 0, sizeof(m_strData));
            
        if (olddata != NULL)
        {
            memcpy(m_strData, olddata, m_nDataPointer);
            delete [] olddata;
        }
    }
}

