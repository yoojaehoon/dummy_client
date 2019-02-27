//#include <mysql.h>
#include "header.h"
#include "Packet.h"
//#include "Database.h"
#include "PerfInfo.h"

#include "JsonParser.h"
#include "RestClient.h"

using namespace std;

#include <deque>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int SERVER_INDEX;
char WMI_MANAGER_IP[128];
int WMI_MANAGER_PORT;
char WMI_MANAGER_LOG_PATH[256];
int PROCESS_POOL_SIZE;
char SERVER_TYPE[16];
char SERVER_UUID[64];

typedef std::deque<char* > ip_deque;

struct ConnInfo
{
    char* ip;
    int port;
    int server_type;

    char* external_ip;
    char* availability_zone;
    char* host;
    char* metaname;
    char* uuid;
    int *thread;
};

sem_t g_processPoolSemaphore;
pthread_mutex_t g_processPoolLock = PTHREAD_MUTEX_INITIALIZER;
deque<int *> g_processPoolQueue;

pthread_mutex_t g_folderLock = PTHREAD_MUTEX_INITIALIZER;

#include "main_report.cpp"
#include "main_alive.cpp"

#include "main_clean.cpp"

int main(int argc, char *argv[])
{
    FILE *file;
    file = (FILE *)fopen("/opt/pub_watcher/conf/pubwatcher.conf", "r");

    if (file == NULL)
    {
        fprintf(stderr, "conf/pubwatcher.conf read failed\n");
        exit(EXIT_FAILURE);
    }

    char buffer[500];
    int confcount = 0;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        fscanf(file, "%[^\n]\n", buffer);

        if (strlen(buffer) == 0)
            break;

        switch (confcount)
        {
            case 0:
                sscanf(buffer, "%d", &SERVER_INDEX);
                break;

            case 1:
                sscanf(buffer, "%s", WMI_MANAGER_IP);
                break;

            case 2:
                sscanf(buffer, "%d", &WMI_MANAGER_PORT);
                break;

            case 3:
                sscanf(buffer, "%s", WMI_MANAGER_LOG_PATH);
                break;

            case 4:
                sscanf(buffer, "%d", &PROCESS_POOL_SIZE);
                break;
            case 5:
                sscanf(buffer, "%s", SERVER_TYPE);
                break;
        }

        confcount++;
    }

    fclose(file);

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    signal(SIGPIPE, SIG_IGN);

    CMiscUtil::InitLog(WMI_MANAGER_LOG_PATH);

    CPerfInfo::Init(SERVER_INDEX, WMI_MANAGER_IP, WMI_MANAGER_PORT);

    pthread_t c_thread;
    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    int result = sem_init(&g_processPoolSemaphore, 0, 0);

    if (result)
    {
        CMiscUtil::WriteErrorLog("WMI Monitor Manager Process Pool Semaphore Process failed", -1, 0);
        exit(EXIT_SUCCESS);
    }

    struct ConnInfo conninfo;
    conninfo.ip = (char*)WMI_MANAGER_IP;
    conninfo.port = WMI_MANAGER_PORT;
    if( strcmp("VM",SERVER_TYPE))
    {
        conninfo.server_type=FALSE;
    }
    else
    {
        conninfo.server_type=TRUE;
    }

    //////////
    CJsonParser w_json;
    RestClient::response res;
    std::string url;
    multimap<string, string> header;
    url = "http://169.254.169.254/openstack/latest/meta_data.json";
    res = RestClient::get(url, header);

    char* body = new char[res.body.size() + 1];
    std::copy(res.body.begin(),res.body.end(), body);
    body[res.body.size()] = '\0';

    w_json.InitData(body, FALSE);
    w_json.ReadParse();

    char uuid[128];
    char availability_zone[32];
    char host[32];
    char metaname[32];

    memset(uuid, 0x00, sizeof(uuid));
    memset(availability_zone,0x00, sizeof(availability_zone));
    memset(host,0x00, sizeof(host));
    memset(metaname,0x00, sizeof(metaname));

    if(w_json.ReadValueString("uuid",uuid)){
        conninfo.uuid = (char*)uuid;
        CMiscUtil::WriteLog("Meta UUID : ", -1, 3, conninfo.uuid);
    }

    if(w_json.ReadValueString("availability_zone",availability_zone)){
        conninfo.availability_zone = (char*)availability_zone;
        CMiscUtil::WriteLog("Meta Availability_zone", -1, 3, conninfo.availability_zone);
    }

    if(w_json.ReadValueString("hostname",host)){
        conninfo.host = (char*)host;
        CMiscUtil::WriteLog("Meta Hostname", -1, 3, conninfo.host);
    }

    if(w_json.ReadValueString("name",metaname)){
        conninfo.metaname = (char*)metaname;
        CMiscUtil::WriteLog("VM Name", -1, 3, conninfo.metaname);
    }

    delete[] body;

    std::string ipurl = "http://bot.whatismyipaddress.com";
    res = RestClient::get(ipurl, header);

    body = new char[res.body.size() + 1];
    std::copy(res.body.begin(),res.body.end(), body);
    body[res.body.size()] = '\0';

    if ( (res.body.end() - res.body.begin()) > 0){
        conninfo.external_ip = (char*)body;
        CMiscUtil::WriteLog("myip : ", -1, 3, body);
    }

    CMiscUtil::WriteLog("MetadataURL : ", -1, 4, url.c_str(), res.body.c_str());
    

    conninfo.external_ip = "none";

    /*struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    ip_deque ipdq;

    char ipbuffer[256];
    memset(ipbuffer,0x00,sizeof(ipbuffer));
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
                strcat(ipbuffer,tmp);
            }
        }
    }
    freeifaddrs(ifap);

    //ipbuffer[-1] = '\0';
    CMiscUtil::WriteLog("Client ip ", -1, 3, ipbuffer);
    conninfo.client_ip = ipbuffer;
    */
    //////////    

    result = pthread_create(&c_thread, &thread_attr, clean_thread, NULL);
    if (result != 0)
    {
        CMiscUtil::WriteErrorLog("Clean Thread Create Error Log", -1, 0);
        exit(EXIT_SUCCESS);
    }

    for (int i = 0; i< PROCESS_POOL_SIZE; i++)
    {
        int *thread = new int;
        *thread = i+1;

        conninfo.thread = thread;
        //result = pthread_create(&c_thread, &thread_attr, alive_process_thread, (void*)thread);
        result = pthread_create(&c_thread, &thread_attr, alive_process_thread, (void*)&conninfo);

        if (result)
        {
            CMiscUtil::WriteErrorLog("WMI Monitor Manager Process Thread Process failed", -1, 0);
            exit(EXIT_SUCCESS);
        }
    }

    //result = pthread_create(&c_thread, &thread_attr, report_perf_thread, (void*)thread);
    result = pthread_create(&c_thread, &thread_attr, report_perf_thread, NULL);

    while(1){
        sleep(10);
    }

    exit(EXIT_SUCCESS);
}
