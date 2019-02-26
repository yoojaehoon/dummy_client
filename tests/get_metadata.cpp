#include<curl/curl.h>
#include<stdio.h>
#include<string.h>

size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, void *s)
{
    size_t written;
    written = fwrite(contents, size, nmemb, (FILE*)s);
    return written;
}

int main(void)
{
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    if(curl) {
        char* data;
        curl_easy_setopt(curl, CURLOPT_URL, "http://169.254.169.254");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
        res = curl_easy_perform(curl);

        printf("curl : %s", data);
        curl_easy_cleanup(curl);
    }

    
}
