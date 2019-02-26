#pragma once

#define FILE_BUFFER_SIZE 64 * 1024
#define PACKET_SIZE 4096
#define SOCKET_BUFFER_SIZE 64 * 1024

#define PROTOCOL_VER         0x01

// Category

#define CAT_REQUEST          1      // 
#define CAT_RESPONSE         2      // 
#define REQ_LOGIN               0x01    // 
#define REQ_QUERY_PATH          0x02    // 
#define REQ_REGISTRY_HEARTBEAT  0x03
#define REQ_ALIVE_STATUS        0x04
#define REQ_REFRESH_INFO        0x05
#define REQ_METADATA_REPORT	0x06

#define REQ_AUTH_REQUEST        0x11    // 
#define REQ_AUTH_END            0x12    // 
#define REQ_DOWNLOAD            0x13    // 
#define REQ_FREE_DOWNLOAD       0x14    // 

#define REQ_FILE_UPLOAD         0x21    // 
#define REQ_FILE_REUPLOAD       0x22    // 
#define REQ_FILE_DELETE         0x23    // 

#define REQ_HASH_CHECK          0x26    // 
#define REQ_FILE_INFO_INSERT    0x27    // 

#define REQ_CONNECT_CNT         0x31    // 
#define REQ_PERF_REPORT         0x32    // 
#define REQ_SERVER_RESTART      0x33    // 


#define RSP_LOGIN               0x01    // 
#define RSP_QUERY_PATH          0x02    // 
#define RSP_REGISTRY_HEARTBEAT  0x03
#define RSP_ALIVE_STATUS        0x04
#define RSP_REFRESH_INFO        0x05
#define RSP_METADATA_REPORT	0x06

#define RSP_AUTH_REQUEST        0x11    // 
#define RSP_AUTH_END            0x12    // 
#define RSP_DOWNLOAD            0x13    // 
#define RSP_FREE_DOWNLOAD       0x14    // 

#define RSP_FILE_UPLOAD         0x21    // 
#define RSP_FILE_REUPLOAD       0x22    // 
#define RSP_FILE_DELETE         0x23    // 

#define RSP_HASH_CHECK          0x26    // 
#define RSP_FILE_INFO_INSERT    0x27    // 

#define RSP_CONNECT_CNT         0x31    // 
#define RSP_PERF_REPORT         0x32    // 
#define RSP_SERVER_RESTART      0x33    // 


#define ERR_NONE             0      // 
#define ERR_FAILED           1      // 
#define ERR_DB_FAILED        2      // 
#define ERR_DISK_ERROR       4      // 
#define ERR_CACHE_SERVER     6      // 
#define ERR_AUTH_FAILED      7      // 
