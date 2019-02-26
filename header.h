#pragma once

#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <signal.h>
#include <sys/resource.h>
#include <numeric>
#include <deque>

#include "packet_common.h"
#include "MiscUtil.h"

using namespace std;

#define TRUE 1
#define FALSE 0
