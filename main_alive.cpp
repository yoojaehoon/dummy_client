int command_heartbeat_report(struct ConnInfo* conninfo);
int command_alive_report(struct ConnInfo* conninfo);
int command_refresh_info(struct ConnInfo* conninfo);
int command_metadata_report(struct ConnInfo* conninfo);

#define REFRESH_INTERVAL    60 * 60
#define METADATA_INTERVAL   60


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

int CloseToServer(int sock)
{
    return !close(sock);
}

void *alive_process_thread(void *arg)
{

    int ret = 0;
    int ret2 = 0;
    int refresh = 0;
    int metareport = 0;
    struct ConnInfo *conninfo;
    const char* ip;
    int port, server_type;
    const char* uuid;
    const char* availability_zone;
    const char* host;
    const char* external_ip;
    //const char* client_ip;

    conninfo = (struct ConnInfo* )arg;

    ip = conninfo->ip;
    port = conninfo->port;
    server_type = conninfo->server_type;
    //client_ip = conninfo->client_ip;
    uuid      = conninfo->uuid;
    availability_zone = conninfo->availability_zone;
    host = conninfo->host;
    external_ip = conninfo->external_ip;

    //client_ip = "1.2.3.4";
    //uuid = "UUID";
    
    CMiscUtil::WriteLog("Start Alive", -1, 0);
    while(TRUE)
    {
        if (ret == FALSE)
            ret = command_heartbeat_report(conninfo);

        ret2 = command_alive_report(conninfo);

        if(refresh < REFRESH_INTERVAL)
            refresh++;
        else{
            ret = command_heartbeat_report(conninfo);
            CMiscUtil::WriteLog("Refresh Client Informs packet send", -1, 0);
            refresh=0;
        }

	if(metareport < METADATA_INTERVAL)
	    metareport++;
	else{
	    metareport = 0;
	    ret = command_metadata_report(conninfo);
	    CMiscUtil::WriteLog("Metadata healthcheck report", -1, 0);
	}

        sleep(1);
    }

    pthread_exit(NULL);
}

int command_metadata_report(struct ConnInfo* conninfo)
{
    int sock = 0;
    int errorcode = 0;
    for (int i=0; i<100; i++)
    {
        if( sock == 0)
        {
            if( !ConnectToServer(conninfo->ip, conninfo->port, &sock))
            {
                CMiscUtil::WriteErrorLog("Connect Error", sock, 0);
                sock = 0;
            }
        }
        else
            break;

        if( i == 99)
        {
            CloseToServer(sock);
            CMiscUtil::WriteErrorLog("Connect try count exceed - command_metadata_report", -1, 0);
            return FALSE;
        }
    }

    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_METADATA_REPORT, sock);

    //included in packet ( external_ip , metadata_health , hostname)

    char external_ip[64];
    int meta_health = TRUE;

    RestClient::response res;
    std::string url;
    multimap<string, string> header;
    url = "http://169.254.169.254/openstack/latest/meta_data.json";
    res = RestClient::get(url, header);

    if(res.code == 0)
        meta_health = FALSE;

    url = "http://bot.whatismyipaddress.com";
    res = RestClient::get(url, header);
    if(res.code != 0){
    	std::copy(res.body.begin(),res.body.end(), external_ip);
        sprintf(external_ip,"%s",res.body.c_str());
    }
    else{
        sprintf(external_ip,"none");
    }
   
    if( !send_packet.WriteString((char*)conninfo->uuid))
    {
        CloseToServer(sock);
        return FALSE;
    }

    if( !send_packet.WriteString((char*)external_ip))
    {
        CloseToServer(sock);
        return FALSE;
    }

    if( !send_packet.WriteInt(meta_health))
    {
        CloseToServer(sock);
        return FALSE;
    }

    if( !send_packet.SendPacket())
    {
        CloseToServer(sock);
        return FALSE;
    }

    CPacket receive_packet;
    receive_packet.InitPacket(sock);

    if( !receive_packet.ReceivePacket())
    {
        CloseToServer(sock);
        return FALSE;
    }

    if( !receive_packet.ReadInt(errorcode))                                                              
    {   
        CloseToServer(sock);                                                                             
        return FALSE;                                                                                    
    }
    if( !receive_packet.IsEOD())                                                                         
    {   
        CloseToServer(sock);                                                                             
        return FALSE;                                                                                    
    }                                                                                                    
    
    if( errorcode == ERR_FAILED)                                                                         
    {   
        CloseToServer(sock);                                                                             
        return FALSE;                                                                                    
    }                                                                                                    
    
    CloseToServer(sock);

    return TRUE;
}

int command_heartbeat_report(struct ConnInfo* conninfo)
{

    int sock = 0;
    for (int i=0; i<100; i++)
    {
        if( sock == 0)
        {
            if( !ConnectToServer(conninfo->ip, conninfo->port, &sock))
            {
                CMiscUtil::WriteErrorLog("Connect Error", sock, 0);
                sock = 0;
            }
        }
        else
            break;

        if( i == 99)
        {
            CloseToServer(sock);
            CMiscUtil::WriteErrorLog("Connect try count exceed - command_heartbeat_report", -1, 0);
            return FALSE;
        }
    }

    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_REGISTRY_HEARTBEAT, sock);

    //char host[128];
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

    if( !send_packet.WriteString((char*)conninfo->metaname))
    {
        CloseToServer(sock);
        return FALSE;
    }

    if( !send_packet.WriteString((char*)conninfo->host))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !send_packet.WriteInt(conninfo->server_type))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !send_packet.WriteString((char*)conninfo->uuid))
    {
        CloseToServer(sock);
        return FALSE;
    }
    //if( !send_packet.WriteString((char*)client_ip))
    if( !send_packet.WriteString((char*)l_szclient_ips))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !send_packet.WriteString((char*)conninfo->availability_zone))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !send_packet.WriteString((char*)conninfo->external_ip))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !send_packet.SendPacket())
    {
        CloseToServer(sock);
        return FALSE;
    }

    CPacket receive_packet;
    receive_packet.InitPacket(sock);

    if( !receive_packet.ReceivePacket())
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !receive_packet.ReadInt(errorcode))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !receive_packet.IsEOD())
    {
        CloseToServer(sock);
        return FALSE;
    }

    if( errorcode == ERR_FAILED)
    {
        CloseToServer(sock);
        return FALSE;
    }

    CloseToServer(sock);
    CMiscUtil::WriteLog("CMD HEARTBEAT Response", sock, 1, &errorcode);
    return TRUE;
}

int command_alive_report(struct ConnInfo* conninfo)
{

    int sock = 0;
    for (int i=0; i<100; i++)
    {
        if( sock == 0)
        {
            if( !ConnectToServer(conninfo->ip, conninfo->port, &sock))
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
            CloseToServer(sock);
            CMiscUtil::WriteErrorLog("Connect try count exceed", -1, 0);
            return FALSE;
        }
    }

    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_ALIVE_STATUS, sock);

    int errorcode = 0;

    if( !send_packet.WriteString((char*)conninfo->metaname))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !send_packet.WriteString((char*)conninfo->uuid))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !send_packet.SendPacket())
    {
        CloseToServer(sock);
        return FALSE;
    }

    CPacket receive_packet;
    receive_packet.InitPacket(sock);

    if( !receive_packet.ReceivePacket())
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !receive_packet.ReadInt(errorcode))
    {
        CloseToServer(sock);
        return FALSE;
    }
    if( !receive_packet.IsEOD())
    {
        CloseToServer(sock);
        return FALSE;
    }

    if( errorcode == ERR_FAILED)
    {
        CloseToServer(sock);
        return FALSE;
    }


    CloseToServer(sock);

    return TRUE;   
}

int command_refresh_info(struct ConnInfo* conninfo)
{
    int sock = 0;
    int errorcode = 0;
    for (int i=0; i<100; i++)
    {
        if( sock == 0)
        {
            if( !ConnectToServer(conninfo->ip, conninfo->port, &sock))
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
            CloseToServer(sock);
            return FALSE;
        }
    }

    CPacket send_packet;
    send_packet.InitPacket(CAT_REQUEST, REQ_REFRESH_INFO, sock);

    CloseToServer(sock);

    CMiscUtil::WriteLog("CMD REFRESH_INFO Response", sock, 1, &errorcode);
    return TRUE;
}

