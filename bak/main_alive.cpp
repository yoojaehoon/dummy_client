int command_heartbeat_report(int client_sockfd, int server_type, const char* uuid);
int command_alive_report(int client_sockfd, int server_type, const char* uuid);
int command_refresh_info(int client_sockfd, int server_type, const char* uuid);

#define REFRESH_INTERVAL    3600

int ConnectToServer(const char* ip , int port, int* sock)
{
    int result;

    *sock = socket(AF_INET, SOCK_STREAM, 0);

    if( *sock == -1)
        return FALSE;

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip);
    server_address.sin_port = htons(port);

    if( connect(*sock, (sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        CMiscUtil::WriteErrorLog("ConnectToServer() - NOT CONNECT", *sock, 0);
        close(*sock);
        return FALSE;
    }

    int optionval = TRUE;
    result = setsockopt(*sock, SOL_SOCKET, SO_KEEPALIVE, &optionval, sizeof(int));

    optionval = TRUE;
    result = setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, &optionval, sizeof(int));

    optionval = SOCKET_BUFFER_SIZE;
    result = setsockopt(*sock, SOL_SOCKET, SO_RCVBUF, &optionval, sizeof(int));

    optionval = SOCKET_BUFFER_SIZE;
    result = setsockopt(*sock, SOL_SOCKET, SO_SNDBUF, &optionval, sizeof(int));

    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = 0;
    result = setsockopt(*sock, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

    CMiscUtil::WriteLog("ConnectToServer() SUCCESS", *sock, 0);

    return TRUE;
}

void *alive_process_thread(void *arg)
{
    int sock = 0;

    int ret = 0;
    int ret2 = 0;
    int refreshcnt = 0;
    struct ConnInfo *conninfo;
    const char* ip;
    int port, server_type;
    const char* uuid;
    //const char* client_ip;

    conninfo = (struct ConnInfo* )arg;

    ip = conninfo->ip;
    port = conninfo->port;
    server_type = conninfo->server_type;
    //client_ip = conninfo->client_ip;
    uuid      = conninfo->uuid;
    //client_ip = "1.2.3.4";
    //uuid = "UUID";
    
    CMiscUtil::WriteLog("Start Alive", sock, 0);
    while(TRUE)
    {
        for (int i=0; i<100; i++)
        {
            if( sock == 0)
            {
                if( !ConnectToServer(ip, port, &sock))
                {
                    CMiscUtil::WriteErrorLog("Connect Error", sock, 0);
                    sock = 0;
                }
                //else
                //    break;
            }
            else
                break;

            if( i == 99)
            {
                CMiscUtil::WriteErrorLog("Connect try count exceed", -1, 0);
                sleep(10);
            }
        }
        if (ret == FALSE)
            ret = command_heartbeat_report(sock, server_type, uuid);

        ret2 = command_alive_report(sock, server_type, uuid);

        if(refreshcnt < REFRESH_INTERVAL)
            refreshcnt++;
        else{
            ret2 = command_refresh_info(sock, server_type, uuid);
            CMiscUtil::WriteLog("Refresh Client Informs packet send", sock, 0);
            refreshcnt=0;
        }
        sleep(1);
    }

    pthread_exit(NULL);
}

int command_heartbeat_report(int sock, int server_type, const char* uuid)
{
    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_REGISTRY_HEARTBEAT, sock);

    char host[128];
    int errorcode = 0;

    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    ip_deque ipdq;

    char l_szclient_ips[256];
    memset(l_szclient_ips,0x00,sizeof(l_szclient_ips));
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next){
        if( ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *)ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            if(strcmp("lo",ifa->ifa_name))
            {
                char tmp[64];
                memset(tmp,0x00, sizeof(tmp));
                sprintf(tmp,"%s:%s,",ifa->ifa_name,addr);
                strcat(l_szclient_ips,tmp);
            }
        }
    }
    freeifaddrs(ifap);




    if ( FALSE != gethostname( host, sizeof(host)))
        return FALSE;

    if( !send_packet.WriteString(host))
        return FALSE;
    if( !send_packet.WriteInt(server_type))
        return FALSE;
    if( !send_packet.WriteString((char*)uuid))
        return FALSE;
    //if( !send_packet.WriteString((char*)client_ip))
    if( !send_packet.WriteString((char*)l_szclient_ips))
        return FALSE;
    if( !send_packet.SendPacket())
        return FALSE;

    CPacket receive_packet;
    receive_packet.InitPacket(sock);

    if( !receive_packet.ReceivePacket_Select())
        return FALSE;
    if( !receive_packet.ReadInt(errorcode))
        return FALSE;
    if( !receive_packet.IsEOD())
        return FALSE;

    if( errorcode == ERR_FAILED)
        return FALSE;

    CMiscUtil::WriteLog("CMD HEARTBEAT Response", sock, 1, &errorcode);
    return TRUE;
}

int command_alive_report(int client_sockfd, int server_type, const char* uuid)
{
    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_ALIVE_STATUS, client_sockfd);

    char host[128];
    int errorcode = 0;

    if ( FALSE != gethostname(host, sizeof(host)))
        return FALSE;

    if( !send_packet.WriteString(host))
        return FALSE;
    if( !send_packet.WriteString((char*)uuid))
        return FALSE;
    if( !send_packet.SendPacket())
        return FALSE;

    CPacket receive_packet;
    receive_packet.InitPacket(client_sockfd);

    if( !receive_packet.ReceivePacket_Select())
        return FALSE;
    if( !receive_packet.ReadInt(errorcode))
        return FALSE;
    if( !receive_packet.IsEOD())
        return FALSE;

    if( errorcode == ERR_FAILED)
        return FALSE;

    return TRUE;   
}

int command_refresh_info(int sock, int server_type, const char* uuid)
{
    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_REFRESH_INFO, sock);

    char host[128];
    int errorcode = 0;

    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    ip_deque ipdq;

    char l_szclient_ips[256];
    memset(l_szclient_ips,0x00,sizeof(l_szclient_ips));
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next){
        if( ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *)ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            if(strcmp("lo",ifa->ifa_name))
            {
                char tmp[64];
                memset(tmp,0x00, sizeof(tmp));
                sprintf(tmp,"%s:%s,",ifa->ifa_name,addr);
                strcat(l_szclient_ips,tmp);
            }
        }
    }
    freeifaddrs(ifap);

    if ( FALSE != gethostname( host, sizeof(host)))
        return FALSE;

    if( !send_packet.WriteString(host))
        return FALSE;
    if( !send_packet.WriteInt(server_type))
        return FALSE;
    if( !send_packet.WriteString((char*)uuid))
        return FALSE;
    //if( !send_packet.WriteString((char*)client_ip))
    if( !send_packet.WriteString((char*)l_szclient_ips))
        return FALSE;
    if( !send_packet.SendPacket())
        return FALSE;

    CPacket receive_packet;
    receive_packet.InitPacket(sock);

    if( !receive_packet.ReceivePacket_Select())
        return FALSE;
    if( !receive_packet.ReadInt(errorcode))
        return FALSE;
    if( !receive_packet.IsEOD())
        return FALSE;

    if( errorcode == ERR_FAILED)
        return FALSE;

    CMiscUtil::WriteLog("CMD REFRESH_INFO Response", sock, 1, &errorcode);
    return TRUE;
}

