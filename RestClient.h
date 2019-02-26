#ifndef INCLUDE_RESTCLIENT_H_
#define INCLUDE_RESTCLIENT_H_

#include <curl/curl.h>
#include <json/json.h>
#include <string>
#include <iostream>
#include <map>

using namespace std;
#include "meta.h"

class RestClient
{
  public:
    /** public data structure **/
    // response struct for queries 
    typedef struct
    {
      int code;
      std::string body;
    } response;
    // struct used for uploading data 
    typedef struct
    {
      const char* data;
      size_t length;
    } upload_object;

    /** public methods **/
    // HTTP GET
    static response get(const string& url, multimap<string, string>& header);
    // HTTP POST
    static response post(const string& url, multimap<string, string>& header, const string& data);
    // HTTP PUT
    static response put(const string& url, multimap<string, string>& header, const string& data);
    // HTTP DELETE
    static response del(const string& url, multimap<string, string>& header);

  private:
    // writedata callback function
    static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
    // read callback function
    static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
    static const char* user_agent;
};

#endif  // INCLUDE_RESTCLIENT_H_

