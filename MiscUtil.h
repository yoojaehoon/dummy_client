#ifndef _MISCUTIL_H
#define _MISCUTIL_H

class CMiscUtil
{
    public:

        static void InitLog(const char *logpath);
        static void ClearOldLog();
        static void WriteLog(const char *message, int client_sockfd, int logtype1, const void *param1 = NULL, const void *param2 = NULL);
        static void WriteErrorLog(const char *message, int client_sockfd, int logtype1, const void *param1 = NULL, const void *param2 = NULL);
        
        
    private:
        static char m_strLogPath[256];
        static pthread_mutex_t m_LogLock;
};

#endif //_MISCUTIL_H

