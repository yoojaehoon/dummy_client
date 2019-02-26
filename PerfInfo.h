#ifndef __PERFINFO_H__
#define __PERFINFO_H__

#include <vector>

class CPerfInfo
{
    public:
        static int Init(int serveridx, char* strIP, int monitor_port);
        static void Destroy();

        static void SendToPerf();

    private:
        static int m_connect;
        static int m_connectioncnt;

        static int m_serveridx;
        static char m_monitor_ip[32];
        static char m_localip[32];

        static int m_monitor_port;
        static int m_hSock;

        static pthread_mutex_t m_PerfLock;
        static pthread_mutex_t m_ConnectLock;

        static int SendToRestart();
        static int ConnectToMonitor();
};

#endif //__PERFINFO_H__
