#include "service.h"
#include "event.h"
#include "variant.h"
#include <stdlib.h>

void    service_post_event(int source_id, const char* data)
{
    event_t* new_event = event_create(source_id, 
                                      variant_create_ptr(DT_SERVICE_EVENT_DATA, strdup(data), variant_delete_default));
    event_post(new_event);
}

variant_t*  service_call_method(service_t* service, const char* method_name, ...)
{
    stack_for_each(service->service_methods, method_variant)
    {
        service_method_t* method = (service_method_t*)variant_get_ptr(method_variant);
        if(strcmp(method->name, method_name) == 0)
        {
            va_list args;
            va_start(args, method_name);
            method->eval_callback(method, args);
            break;
        }
    }
}

