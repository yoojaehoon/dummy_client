
void *report_perf_thread(void *arg)
{
    /*int threadnum;
    threadnum = *((int *)arg);
    delete (int *)arg;
    */

    while (1)
    {
        sleep(60);
        CPerfInfo::SendToPerf();
    }

    pthread_exit(NULL);
}
