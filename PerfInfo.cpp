#include "header.h"
#include "PerfInfo.h"
#include "Packet.h"

int CPerfInfo::m_hSock = 0;
int CPerfInfo::m_connect = 0;
int CPerfInfo::m_connectioncnt = 0;
int CPerfInfo::m_serveridx = 0;
char CPerfInfo::m_localip[32] = "";
char CPerfInfo::m_monitor_ip[32] = "";
int CPerfInfo::m_monitor_port = 0;


pthread_mutex_t CPerfInfo::m_ConnectLock;
pthread_mutex_t CPerfInfo::m_PerfLock;

int CPerfInfo::Init(int serveridx, char* monitor_ip, int monitor_port)
{
    m_connectioncnt = 0;
    m_serveridx = serveridx;
    sscanf(monitor_ip, "%s", m_monitor_ip);
    m_monitor_port = monitor_port;

    int result = pthread_mutex_init(&m_ConnectLock, NULL);

    if (result)
    {
        CMiscUtil::WriteErrorLog("CPerfInfo::Init", -1, 3, "Connect pthread_mutex_init");
        return FALSE;
    }

    result = pthread_mutex_init(&m_PerfLock, NULL);

    if (result)
    {
        CMiscUtil::WriteErrorLog("CPerfInfo::Init", -1, 3, "PerfLock pthread_mutex_init");
        return FALSE;
    }

    if( ConnectToMonitor())
    {
        int sock_len;
        struct sockaddr_in sock_address;

        sock_len = sizeof(sock_address);
        getpeername(m_hSock, (struct sockaddr *)&sock_address, (socklen_t *)&sock_len);

        inet_ntop(AF_INET, (void *)&(sock_address.sin_addr), m_localip, sizeof(m_localip));

        if( !SendToRestart())
        {
            CMiscUtil::WriteErrorLog("SERVER_RESTART REPORT FAILED", m_hSock, 0);
        }
    }

    return TRUE;
}

void CPerfInfo::Destroy()
{
    pthread_mutex_destroy(&m_ConnectLock);
    pthread_mutex_destroy(&m_PerfLock);
}

int CPerfInfo::SendToRestart()
{
    int errorcode;

    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_SERVER_RESTART, m_hSock, 0);

    if( !send_packet.WriteInt(m_serveridx))
        return FALSE;
    if( !send_packet.SendPacket())
        return FALSE;

    CPacket receive_packet;
    receive_packet.InitPacket(m_hSock, 0);

    if( !receive_packet.ReceivePacket_Select() )
        return FALSE;
    if( !receive_packet.ReadInt(errorcode))
        return FALSE;

    if( errorcode != ERR_NONE)
    {
        CMiscUtil::WriteErrorLog("SERVER_RESTART REPORT ERRORCODE", m_hSock, 1, &errorcode);
        return FALSE;
    }

    CMiscUtil::WriteLog("SERVER_RESTART REPORT SUCCESS", m_hSock, 1, &errorcode);

    return TRUE;
}

void CPerfInfo::SendToPerf()
{
    int errorcode;
    int connection = 0;

    pthread_mutex_lock(&m_PerfLock);
    connection = m_connectioncnt;

    m_connectioncnt = 0;
    pthread_mutex_unlock(&m_PerfLock);

    if( m_hSock == 0)
    {
        if( !ConnectToMonitor())
        {
            m_hSock = 0;
            return;
        }
    }

    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_PERF_REPORT, m_hSock, 0);

    if( !send_packet.WriteInt(m_serveridx) )
    {
        close(m_hSock);
        m_hSock = 0;
        return;
    }

    if( !send_packet.WriteInt(connection))
    {
        close(m_hSock);
        m_hSock = 0;
        return;
    }

    if( !send_packet.SendPacket())
    {
        close(m_hSock);
        m_hSock = 0;
        return;
    }

    CPacket receive_packet;
    receive_packet.InitPacket(m_hSock, 0);

    if( !receive_packet.ReceivePacket_Select())
    {
        close(m_hSock);
        m_hSock = 0;
        return;
    }

    if( !receive_packet.ReadInt(errorcode))
    {
        close(m_hSock);
        m_hSock = 0;
        return;
    }

    if( errorcode != ERR_NONE)
    {
        CMiscUtil::WriteErrorLog("Performance report Error", m_hSock, 1, &errorcode);
    }

    CMiscUtil::WriteLog("Return REQ_PERF_REPORT", m_hSock, 1, &errorcode);
    return;
}

int CPerfInfo::ConnectToMonitor()
{
    int result;

    m_hSock = socket(AF_INET, SOCK_STREAM, 0);

    if( m_hSock == -1 )
        return FALSE;

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(m_monitor_ip);
    server_address.sin_port = htons(m_monitor_port);

    if( connect(m_hSock, (sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        CMiscUtil::WriteErrorLog("ConnectToMonitor() - NOT CONNECT", m_hSock, 0);
        close(m_hSock);
        return FALSE;
    }

    int optionval = TRUE;
    result = setsockopt(m_hSock, SOL_SOCKET, SO_KEEPALIVE, &optionval, sizeof(int));

    optionval = TRUE;
    result = setsockopt(m_hSock, IPPROTO_TCP, TCP_NODELAY, &optionval, sizeof(int));

    optionval = SOCKET_BUFFER_SIZE;
    result = setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, &optionval, sizeof(int));

    optionval = SOCKET_BUFFER_SIZE;
    result = setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, &optionval, sizeof(int));

    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = 0;
    result = setsockopt(m_hSock, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

    CMiscUtil::WriteLog("ConnectToMonitor() SUCCESS", m_hSock, 0);
    return TRUE;
}
