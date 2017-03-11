#include "service.h"
#include "event.h"
#include "variant.h"
#include <stdlib.h>
#include "hash.h"
#include "crc32.h"

hash_table_t* service_table;

void    delete_service_event_data(void* arg)
{
    service_event_data_t* e = (service_event_data_t*)arg;
    free(e->data);
    free(e);
}

void    service_post_event(int service_id, const char* name, variant_t* data)
{
    event_t* new_event = event_create(service_id, name, variant_create_ptr(DT_SERVICE_EVENT_DATA, data, delete_service_event_data));
    event_post(new_event);
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
