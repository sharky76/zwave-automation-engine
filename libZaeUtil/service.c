#include "service.h"
#include "event.h"
#include "variant.h"
#include <stdlib.h>
#include "hash.h"
#include "crc32.h"
#include "event_pump.h"

hash_table_t* service_table;

void    delete_service_event_data(void* arg)
{
    service_event_data_t* e = (service_event_data_t*)arg;
    free(e->data);
    free(e);
}

/**
 * Post event from service. Note that even manager will try to 
 * delete data variant 
 * 
 * @author alex (3/12/2017)
 * 
 * @param service_id 
 * @param name 
 * @param data 
 */

void service_event_free(void* event)
{
    event_t* e = (event_t*)event;
    free(e->data);
    free(e);
}

void    service_post_event(int service_id, int event_id, void* data)
{
    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    event_t* new_event = (event_t*)malloc(sizeof(event_t));
    new_event->data = data;
    new_event->source_id = service_id;
    event_pump_send_event(pump, event_id, new_event, &service_event_free);
}

variant_t*  service_call_method(const char* service_name, const char* method_name, ...)
{
    variant_t* ret = NULL;
    service_t* service = service_self(service_name);
    stack_for_each(service->service_methods, method_variant)
    {
        service_method_t* method = (service_method_t*)variant_get_ptr(method_variant);
        if(strcmp(method->name, method_name) == 0)
        {
            va_list args;
            va_start(args, method_name);
            ret = method->eval_callback(method, args);
            break;
        }
    }

    return ret;
}

service_t*  service_self(const char* service_name)
{
    uint32_t key = crc32(0, service_name, strlen(service_name));
    variant_t* service_variant = variant_hash_get(service_table, key);
    return variant_get_ptr(service_variant);
}

service_t*  service_by_id(int service_id)
{
    hash_iterator_t* it = variant_hash_begin(service_table);
    service_t* retVal = NULL;

    while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
    {
        service_t* service = (service_t*)variant_get_ptr(variant_hash_iterator_value(it));

        if(service->service_id == service_id)
        {
            retVal = service;
            break;
        }
    }

    free(it);
    return retVal;
}

variant_t*  service_eval_method(service_method_t* method, ...)
{
    variant_t*  retVal = NULL;
    va_list args;
    va_start(args, method);

    return method->eval_callback(method, args);
}

