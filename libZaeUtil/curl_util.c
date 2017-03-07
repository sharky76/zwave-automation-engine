#include "curl_util.h"
#include <string.h>

typedef struct response_memory_t 
{
    char    *memory;
    size_t  size;
} response_memory_t;

size_t write_cb(char *in, size_t size, size_t nmemb, void *out)
{
    size_t realsize = size * nmemb;
    response_memory_t *mem = (response_memory_t *)out;
 
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) 
    {
        /* out of memory! */ 
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
 
    memcpy(&(mem->memory[mem->size]), in, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
 
    return realsize;
}

void    curl_util_get_json(const char* request_url, void (response_parser)(const json_object*, void*), void* arg)
{
    response_memory_t chunk;
    CURL *curl_handle;
    CURLcode res;

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    /* no data at this point */ 

    curl_global_init(CURL_GLOBAL_ALL);
    
    /* init the curl session */ 
    curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, request_url);
    
    /* send all data to this function  */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb);
    
    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    
    /* some servers don't like requests that are made without a user-agent
    field, so we provide one */ 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30);

    //curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 4L);
    /* get it! */ 
    res = curl_easy_perform(curl_handle);
    
    /* check for errors */ 
    if(res != CURLE_OK) 
    {
    }
    else 
    {
        struct json_object* response_obj = json_tokener_parse(chunk.memory);

        if(NULL != response_parser)
        {
            response_parser(response_obj, arg);
        }

        json_object_put(response_obj);
    }
    
    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl_handle);
    
    free(chunk.memory);
    
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
}
