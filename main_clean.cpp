
void *clean_thread(void *arg)
{
    while(1)
    {
        CMiscUtil::ClearOldLog();
        sleep(3600);
    }
    pthread_exit(NULL);
}
