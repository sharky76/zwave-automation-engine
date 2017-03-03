#include "service.h"
#include "ifttt_cli.h"
#include "ifttt_config.h"
#include "logger.h"
#include "service_args.h"
#include <curl/curl.h>
#include <json-c/json.h>
#include "crc32.h"

int DT_IFTTT;
char*    ifttt_key;

#define IFTTT_MAKER_URL "https://maker.ifttt.com/trigger/%s/with/key/%s"

variant_t* ifttt_trigger_event(service_method_t* method, va_list args);

void    service_create(service_t** service, int service_id)
{
    SERVICE_INIT(IFTTT, "Send and receive IFTTT triggers");
    SERVICE_ADD_METHOD(Trigger, ifttt_trigger_event, 1, "Trigger IFTTT event. Arg: Event, 3 Values can be provided as argument stack.");

    (*service)->get_config_callback = ifttt_cli_get_config;
    DT_IFTTT = service_id;
}

void    service_cli_create(cli_node_t* parent_node)
{
    ifttt_cli_create(parent_node);
}

variant_t* ifttt_trigger_event(service_method_t* method, va_list args)
{
    variant_t* event_name = va_arg(args, variant_t*);
    service_args_stack_t* service_stack = service_args_stack_get("IFTTT.Trigger");
    hash_table_t*   value_table = (hash_table_t*)service_stack->data_storage;
    
    if(NULL == ifttt_key)
    {
        LOG_ERROR(DT_IFTTT, "Key not set");
        return variant_create_bool(false);
    }

    // Create JSON payload
    json_object* payload_obj = json_object_new_object();
    char* json_keys[3] = {"value1", "value2", "value3"};


    for(int i = 0; i < 3; i++)
    {
        variant_t* value_var;
        uint32_t key = crc32(0, json_keys[i], strlen(json_keys[i]));
        if((value_var = variant_hash_get(value_table, key)) != NULL)
        {
            json_object *value_obj = json_object_new_string(variant_get_string(value_var));
            json_object_object_add(payload_obj, json_keys[i], value_obj);
        }
    }

    const char* payload_str = json_object_to_json_string_ext(payload_obj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
    LOG_ADVANCED(DT_IFTTT, "Creating payload %s", payload_str);

    CURL *curl_handle;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */ 
    curl_handle = curl_easy_init();

    if(NULL != curl_handle)
    {
        char urlbuf[512] = {0};
    
        /* specify URL to get */ 
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
     
        /* If the site you're connecting to uses a different host name that what
         * they have mentioned in their server certificate's commonName (or
         * subjectAltName) fields, libcurl will refuse to connect. You can skip
         * this check, but this will make the connection less secure. */ 
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

        snprintf(urlbuf, 511, IFTTT_MAKER_URL, variant_get_string(event_name), ifttt_key);
        
        LOG_DEBUG(DT_IFTTT, "Trigger URL is %s", urlbuf);
        curl_easy_setopt(curl_handle, CURLOPT_URL, urlbuf);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, payload_str);
    
        struct curl_slist *headers=NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        /* get it! */ 
        res = curl_easy_perform(curl_handle);
        
        curl_slist_free_all(headers);

        /* check for errors */ 
        if(res != CURLE_OK) 
        {
            LOG_ERROR(DT_IFTTT, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        }
        else 
        {
            LOG_DEBUG(DT_IFTTT, "Mail sent successfully");
        }
    
        /* cleanup curl stuff */ 
        curl_easy_cleanup(curl_handle);
    }

    return variant_create_bool(true);
}
