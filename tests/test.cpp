#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

struct url_data {
    size_t size;
    char* data;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, url_data *data) {
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmpData;

    data->size += (size * nmemb);

    tmpData = (char*)realloc(data->data, data->size + 1);

    if(tmpData)
    {
        data->data = tmpData;
    }
    else
    {
        if(data->data){
            free(data->data);
        }
        printf("faild to allocate memory");
        return 0;
    }

    memcpy((data->data + index), ptr, n);
    data->data[data->size] = '\0';
    //written = fwrite(ptr, size, nmemb, stream);
    return size * nmemb;
}

int main(void) {
    CURL *curl;
    struct url_data data;
    data.size = 0;
    data.data = (char*)malloc(4096);

    if(NULL == data.data){
        printf("Faild to allocate memory\n");
        return NULL;
    }

    data.data[0] = '\0';
    CURLcode res;
    char *url = "169.254.169.254";
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",   curl_easy_strerror(res));

        curl_easy_cleanup(curl);

        //printf("\n%s", fp);
    }
    else{
        curl_easy_cleanup(curl);
    }

    printf("%s\n", data.data);

    data.size = 0;
    data.data = (char*)malloc(4096);

    if(NULL == data.data){
        printf("Failed to allocate memory2\n");
        return NULL;
    }

    data.data[0] = '\0';
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "bot.whatismyipaddress.com");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    printf("%s", data.data);
}


