#include "weather_cache.h"
#include "weather_config.h"
#include <curl/curl.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <logger.h>

weather_cache_t  weather_cache;
void    weather_cache_parse_response(struct json_object* weather_response_obj);

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

void weather_cache_init()
{
    weather_cache.cache_age = 0;
    weather_cache.windspeed = 0;
    weather_cache.temp = 0;
    weather_cache.precipitation = NULL;
}

void    weather_cache_refresh()
{
    time_t query_time = time(NULL);

    if(query_time - weather_cache.cache_age < 600)
    {
        LOG_DEBUG(DT_WEATHER, "Weather cache is current");
        return;
    }

    weather_cache.cache_age = query_time;
       
    response_memory_t chunk;
    CURL *curl_handle;
    CURLcode res;

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    /* no data at this point */ 

    curl_global_init(CURL_GLOBAL_ALL);
    
    /* init the curl session */ 
    curl_handle = curl_easy_init();
    
    /* specify URL to get */ 
    char urlbuf[512] = {0};
    snprintf(urlbuf, 511, "http://api.openweathermap.org/data/2.5/weather?zip=%d,%s&units=%s&APPID=%s", weather_zip, weather_country_code, weather_temp_units, "337b07da05ad8ebf391e2252f02196cf");

    LOG_DEBUG(DT_WEATHER, "Weather URL %s", urlbuf);

    curl_easy_setopt(curl_handle, CURLOPT_URL, urlbuf);
    
    /* send all data to this function  */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb);
    
    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    
    /* some servers don't like requests that are made without a user-agent
    field, so we provide one */ 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    /* get it! */ 
    res = curl_easy_perform(curl_handle);
    
    /* check for errors */ 
    if(res != CURLE_OK) 
    {
     fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
    }
    else 
    {
        LOG_DEBUG(DT_WEATHER, "%lu bytes retrieved", (long)chunk.size);
        struct json_object* weather_response_obj = json_tokener_parse(chunk.memory);
        weather_cache_parse_response(weather_response_obj);
    }
    
    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl_handle);
    
    free(chunk.memory);
    
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
}

void    weather_cache_parse_response(struct json_object* weather_response_obj)
{
    //printf("jobj from str:\n---\n%s\n---\n", json_object_to_json_string_ext(weather_response_obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    free(weather_cache.precipitation);
    weather_cache.precipitation = 0;

    json_object_object_foreach(weather_response_obj, key, value)
    {
        if(strcmp(key, "weather") == 0)
        {
            int weather_entries = json_object_array_length(value);
            for(int i = 0; i < weather_entries; i++)
            {
                struct json_object* weather_entry_obj = json_object_array_get_idx(value, i);
                struct json_object* weather_value;
                if(json_object_object_get_ex(weather_entry_obj, "main", &weather_value) == TRUE)
                {
                    const char* weather_string = json_object_get_string(weather_value);
                    int old_len = 0;
                    if(NULL != weather_cache.precipitation)
                    {
                        old_len = strlen(weather_cache.precipitation);
                    }
                    weather_cache.precipitation = realloc(weather_cache.precipitation, (old_len + 1 + strlen(weather_string))*sizeof(char));
                    if(NULL != weather_cache.precipitation)
                    {
                        memset(weather_cache.precipitation + (old_len), 0, 1 + strlen(weather_string));
                        strcat(weather_cache.precipitation, weather_string);

                        if(i < weather_entries-1)
                        {
                            strcat(weather_cache.precipitation, "|");
                        }
                    }
                }
            }
        }
        else if(strcmp(key, "main") == 0)
        {
            struct json_object* temp_data;
            if(json_object_object_get_ex(value, "temp", &temp_data) == TRUE)
            {
                weather_cache.temp = json_object_get_double(temp_data);
            }
            struct json_object* humidity_data;
            if(json_object_object_get_ex(value, "humidity", &humidity_data) == TRUE)
            {
                weather_cache.humidity = json_object_get_int(humidity_data);
            }
        }
        else if(strcmp(key, "wind") == 0)
        {
            struct json_object* wind_data;
            if(json_object_object_get_ex(value, "speed", &wind_data) == TRUE)
            {
                weather_cache.windspeed = json_object_get_double(wind_data);
            }
        }
    }

    LOG_DEBUG(DT_WEATHER, "Updated weather values - precipitation: %s, temp: %f, humidity: %d, wind: %f", 
           weather_cache.precipitation,
           weather_cache.temp,
           weather_cache.humidity,
           weather_cache.windspeed);
}

