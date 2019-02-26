#include "RestClient.h"

#include <stdlib.h>
#include <string.h>

/** initialize user agent string */
const char* RestClient::user_agent = "restclient/" VERSION;
RestClient::response RestClient::get(const string& url, multimap<string, string>& header)
{
    RestClient::response ret;
    CURL *curl;
    CURLcode res;
    curl_slist *responseHeaders = NULL;
    for(multimap<string, string>::iterator it=header.begin(); it != header.end(); ++ it)
    {
        string requestheader = (*it).first+": " +(*it).second;
        responseHeaders = curl_slist_append(responseHeaders, requestheader.c_str());
    }

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, RestClient::user_agent);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, responseHeaders);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RestClient::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
        res = curl_easy_perform(curl);
        if (res != 0)
        {
            cerr << "Failed to query " << url << ":"
            << endl << curl_easy_strerror(res) << endl << flush;
            //exit(1);
        }
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        ret.code = static_cast<int>(http_code);

        curl_easy_cleanup(curl);
    }

    return ret;
}

RestClient::response RestClient::post(const string& url,
                                      multimap<string, string>& header,
                                      const string& data)
{
    RestClient::response ret;
    curl_slist *responseHeaders = NULL;
        for(multimap<string, string>::iterator it=header.begin(); it != header.end(); ++ it)
        {
                string requestheader = (*it).first+": " +(*it).second;
                responseHeaders = curl_slist_append(responseHeaders, requestheader.c_str());
        }

    RestClient::upload_object up_obj;
    up_obj.data = data.c_str();
    up_obj.length = data.size();

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, RestClient::user_agent);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RestClient::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, responseHeaders);
        res = curl_easy_perform(curl);
        if (res != 0)
        {
            cerr << "Failed to query " << url << ":"
            << endl << curl_easy_strerror(res) << endl << flush;
            exit(1);
        }
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        ret.code = static_cast<int>(http_code);

        curl_easy_cleanup(curl);
    }

    return ret;
}

RestClient::response RestClient::put(const string& url,
                                     multimap<string, string>& header,
                                     const string& data)
{
    RestClient::response ret;
    curl_slist *responseHeaders = NULL;
        for(multimap<string, string>::iterator it=header.begin(); it != header.end(); ++ it)
        {
                string requestheader = (*it).first+": " +(*it).second;
                responseHeaders = curl_slist_append(responseHeaders, requestheader.c_str());
        }

    RestClient::upload_object up_obj;
    up_obj.data = data.c_str();
    up_obj.length = data.size();

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, RestClient::user_agent);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, RestClient::read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &up_obj);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RestClient::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE, static_cast<long>(up_obj.length));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, responseHeaders);
        res = curl_easy_perform(curl);
        if (res != 0)
        {
            cerr << "Failed to query " << url << ":"
            << endl << curl_easy_strerror(res) << endl << flush;
            exit(1);
        }
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        ret.code = static_cast<int>(http_code);

        curl_easy_cleanup(curl);
    }

    return ret;
}

RestClient::response RestClient::del(const string& url, multimap<string, string>& header)
{
    RestClient::response ret;
    curl_slist *responseHeaders = NULL;
        for(multimap<string, string>::iterator it=header.begin(); it != header.end(); ++ it)
        {
                string requestheader = (*it).first+": " +(*it).second;
                responseHeaders = curl_slist_append(responseHeaders, requestheader.c_str());
        }

    const char* http_delete = "DELETE";

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, RestClient::user_agent);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, responseHeaders);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, http_delete);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RestClient::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
        res = curl_easy_perform(curl);
        if (res != 0)
        {
            cerr << "Failed to query " << url << ":"
             << endl << curl_easy_strerror(res) << endl << flush;
            exit(1);
        }
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        ret.code = static_cast<int>(http_code);

        curl_easy_cleanup(curl);
    }

    return ret;
}

size_t RestClient::write_callback(void *data, size_t size, size_t nmemb, void *userdata)
{
    RestClient::response* r;
    r = reinterpret_cast<RestClient::response*>(userdata);
    r->body.append(reinterpret_cast<char*>(data), size*nmemb);

    return (size * nmemb);
}

size_t RestClient::read_callback(void *data, size_t size, size_t nmemb, void *userdata)
{
    RestClient::upload_object* u;
    u = reinterpret_cast<RestClient::upload_object*>(userdata);
    size_t curl_size = size * nmemb;
    size_t copy_size = (u->length < curl_size) ? u->length : curl_size;
    memcpy(data, u->data, copy_size);
    u->length -= copy_size;
    u->data += copy_size;
    return copy_size;
}
