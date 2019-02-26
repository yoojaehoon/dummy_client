#include "header.h"
#include "MiscUtil.h"

char CMiscUtil::m_strLogPath[256] = "";
pthread_mutex_t CMiscUtil::m_LogLock = PTHREAD_MUTEX_INITIALIZER;

void CMiscUtil::InitLog(const char *logpath)
{
    strcpy(m_strLogPath, logpath);
    
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char newpath[256];
    
    if ((dp = opendir(m_strLogPath)) == NULL)
        return;
   
    time_t the_time;
    (void)time(&the_time);
      
    while ((entry = readdir(dp)) != NULL)
    {
        sprintf(newpath, "%s/%s", m_strLogPath, entry->d_name);
        
        if (lstat(newpath, &statbuf) == -1)
            return;

        if (S_ISREG(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 ||
                strcmp("..", entry->d_name) == 0)
                continue;
            
            if (statbuf.st_mtime < the_time - 30 * 24 * 3600)
                unlink(newpath);
        }
    }
}


void CMiscUtil::ClearOldLog()
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char newpath[256];
    
    if ((dp = opendir(m_strLogPath)) == NULL)
        return;
   
    time_t the_time;
    (void)time(&the_time);
      
    while ((entry = readdir(dp)) != NULL)
    {
        sprintf(newpath, "%s/%s", m_strLogPath, entry->d_name);
        
        if (lstat(newpath, &statbuf) == -1)
            return;

        if (S_ISREG(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 ||
                strcmp("..", entry->d_name) == 0)
                continue;
            
            if (statbuf.st_mtime < the_time - 5 * 24 * 3600)
                unlink(newpath);
        }
    }
        
    closedir(dp);       
}

void CMiscUtil::WriteLog(const char *message, int client_sockfd, int logtype1, const void *param1, const void *param2)
{
    FILE *file;
    struct tm tm_ptr;
    time_t the_time;
    char logpath[256];

//  pthread_mutex_lock(&m_LogLock);

    (void)time(&the_time);
    localtime_r(&the_time, &tm_ptr);

    sprintf(logpath, "%sINFO_%04d-%02d-%02d_%02d", m_strLogPath, tm_ptr.tm_year+1900, tm_ptr.tm_mon+1, tm_ptr.tm_mday, tm_ptr.tm_hour);

    file = (FILE *)fopen(logpath, "a+");

    if (file == NULL)
    {
//      pthread_mutex_unlock(&m_LogLock);
        return;
    }

    struct timeval tv = {0, 0};
    gettimeofday(&tv, 0);
    
    fprintf(file, "[%02d:%02d:%02d %06d] %s", tm_ptr.tm_hour, tm_ptr.tm_min, tm_ptr.tm_sec, (int)tv.tv_usec, message);
    
    if (client_sockfd != -1)
        fprintf(file, ", SOCKET - %d", client_sockfd);
        
    switch (logtype1)
    {
        case 0:
            fprintf(file, "\n");
            break;
            
        case 1:
            fprintf(file, ", P1 - %d\n", *(int *)param1);
            break;
            
        case 2:
            fprintf(file, ", P1 - %lld\n", *(long long *)param1);
            break;
            
        case 3:
            fprintf(file, ", P1 - %.300s\n", (const char *)param1);
            break;
            
        case 4:
            fprintf(file, ", P1 - %.300s, P2 - %.300s\n", (const char *)param1, (const char *)param2);
            break;
    }
                
    fclose(file);

//  pthread_mutex_unlock(&m_LogLock);   
}

void CMiscUtil::WriteErrorLog(const char *message, int client_sockfd, int logtype1, const void *param1, const void *param2)
{
    FILE *file;
    struct tm tm_ptr;
    time_t the_time;
    char logpath[256];

//  pthread_mutex_lock(&m_LogLock);

    (void)time(&the_time);
    localtime_r(&the_time, &tm_ptr);

    sprintf(logpath, "%sERROR_%04d-%02d-%02d", m_strLogPath, tm_ptr.tm_year+1900, tm_ptr.tm_mon+1, tm_ptr.tm_mday);

    file = (FILE *)fopen(logpath, "a+");

    if (file == NULL)
    {
//      pthread_mutex_unlock(&m_LogLock);
        return;
    }
    
    struct timeval tv = {0, 0};
    gettimeofday(&tv, 0);

    fprintf(file, "[%02d:%02d:%02d %06d] %s", tm_ptr.tm_hour, tm_ptr.tm_min, tm_ptr.tm_sec, (int)tv.tv_usec, message);
    
    if (client_sockfd != -1)
        fprintf(file, ", SOCKET - %d", client_sockfd);
    
    switch (logtype1)
    {
        case 0:
            fprintf(file, "\n");
            break;
            
        case 1:
            fprintf(file, ", P1 - %d\n", *(int *)param1);
            break;
            
        case 2:
            fprintf(file, ", P1 - %lld\n", *(long long *)param1);
            break;
            
        case 3:
            fprintf(file, ", P1 - %.300s\n", (const char *)param1);
            break;
            
        case 4:
            fprintf(file, ", P1 - %.300s, P2 - %.300s\n", (const char *)param1, (const char *)param2);
            break;
    }
                
    fclose(file);

//  pthread_mutex_unlock(&m_LogLock);   
}

