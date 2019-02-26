/*
 * LogUtil.h
 *
 *  Created on: 2011. 3. 18.
 *      Author: 윤영빈
 */

#ifndef LOGUTIL_H_
#define LOGUTIL_H_

class CLogUtil
{
    public:
        // 로그 관련 함수
        static void InitLog(char *logpath);
        static void ClearOldLog();
        static void WriteLog(char *message, int client_sockfd, int logtype1, const void *param1 = NULL, const void *param2 = NULL);
        static void WriteErrorLog(char *message, int client_sockfd, int logtype1, const void *param1 = NULL, const void *param2 = NULL);

    private:
        // 로그 관련 변수
        static char m_strLogPath[256];
        static pthread_mutex_t m_LogLock;
};

#endif /* LOGUTIL_H_ */

