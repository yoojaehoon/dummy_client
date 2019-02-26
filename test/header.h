#pragma once

#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <utime.h>
#include <stdio.h>

//#include "thread.h"
#include "JsonParser.h"
#include "RestClient.h"
//#include "LogUtil.h"

using namespace std;

#define TRUE 1
#define FALSE 0
#define MAX_PATH 500

// 오류 코드

#define ERR_NONE             0      // None, Success
#define ERR_FAILED           1      // 실행할 수 없음 (General Failure)
#define ERR_EXIST            2      // 이미 존재함
#define ERR_VERSION          3      // 버전이 일치하지 않음
#define ERR_FILE_NOT_FOUND   4      // 파일이 존재하지 않음

