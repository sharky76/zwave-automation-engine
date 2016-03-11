#include "service.h"
#include "event.h"
#include "variant.h"
#include <stdlib.h>
#include "hash.h"

hash_table_t* service_table;

void    delete_service_event_data(void* arg)
{
    service_event_data_t* e = (service_event_data_t*)arg;
    free(e->data);
    free(e);
}

void    service_post_event(int service_id, const char* data)
{
    service_event_data_t* event_data = calloc(1, sizeof(service_event_data_t));
    event_data->data = strdup(data);

    event_t* new_event = event_create(service_id, 
                                      variant_create_ptr(DT_SERVICE_EVENT_DATA, event_data, delete_service_event_data));
    event_post(new_event);
}

variant_t*  service_call_method(int service_id, const char* method_name, ...)
{
    variant_t* ret = NULL;
    service_t* service = service_self(service_id);
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

service_t*  service_self(int service_id)
{
    variant_t* service_variant = variant_hash_get(service_table, service_id);
    return variant_get_ptr(service_variant);
}

