//#include "stdafx.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "header.h"

#include <deque>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef std::deque<char* > ip_deque;

int main(int argc, char *argv[])
{
    char buffer[1024];
    CJsonParser w_json;
    /*w_json.WriteValueString("name", "mynameisjson");
    w_json.MakeWrite(buffer);

    printf("test json : %s", buffer);
    */
    RestClient::response res;
    std::string url;
    multimap<string, string> header;
    url = "http://169.254.169.253/openstack/latest/meta_data.json";
    printf("url : %s", url.c_str());
    res = RestClient::get(url, header);
    printf("Response[%d] %s\n", res.code, res.body.c_str());

    char* body = new char[res.body.size() + 1];
    std::copy(res.body.begin(),res.body.end(), body);
    body[res.body.size()] = '\0';

    w_json.InitData(body,FALSE);
    w_json.ReadParse();
    
    memset(buffer, 0x00, sizeof(buffer));
    if(w_json.ReadValueString("uuid",buffer))
        printf("ReadValueString: %s\n", buffer);
    else
        printf("Get Failed String\n");
    
    delete[] body;
    //res.body.c_str();

    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;

    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next){
        if( ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *)ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            printf("Interface ip address : %s %s\n", ifa->ifa_name, addr);
        }
    }

    freeifaddrs(ifap);


    char ipbuffer[128];
    memset(ipbuffer, 0x00,sizeof(ipbuffer));
    ip_deque str;
    str.push_back("1.2.3.4");
    str.push_back("11.22.33.44");
    str.push_back("111.222.333.444");

    printf("str size : %d\n", str.size());
    for(ip_deque::iterator iter=str.begin();iter != str.end();iter++)
    {
        printf("%s\n", *iter);
        strcat(ipbuffer,*iter);
        strcat(ipbuffer,",");
    }

    printf("ip_buffer : %s", ipbuffer);
}
