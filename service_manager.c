#include "service_manager.h"
#include <stack.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include "logger.h"
#include "variant_types.h"
#include "cli_logger.h"
#include "cli_service.h"
#include <event.h>
#include <hash.h>
#include <crc32.h>
#include "command_parser.h"

//static variant_stack_t* service_list;
extern hash_table_t* service_table;

DECLARE_LOGGER(ServiceManager)

void service_manager_on_command_activation_event(event_t* event, void* context);

void    service_manager_init(const char* service_dir)
{
    LOG_ADVANCED(ServiceManager, "Initializing service manager...");
    DIR *dp;
    struct dirent *ep;     
    dp = opendir (service_dir);

    void    (*service_create)(service_t**, int);
    void    (*service_cli_create)(cli_node_t* root_node);
    char full_path[256] = {0};
    
    //service_list = stack_create();
    service_table = variant_hash_init();

    if(dp != NULL)
    {
        while (ep = readdir (dp))
        {
            strcat(full_path, service_dir);
            strcat(full_path, "/");
            strcat(full_path, ep->d_name);

            void* handle = dlopen(full_path, RTLD_NOW);
            char* error = dlerror();
                if(NULL != error)
                {
                    LOG_ERROR(ServiceManager, "Error opening service %s: %s", full_path, error);
                }
            if(NULL != handle)
            {
                LOG_DEBUG(ServiceManager, "Loading services from %s", full_path);
                dlerror();
                *(void**) (&service_create) = dlsym(handle, "service_create");

                char* error = dlerror();
                if(NULL != error)
                {
                    LOG_ERROR(ServiceManager, "Error getting service entry point: %s", error);
                }
                else
                {
                    service_t* service;
                    (*service_create)(&service, variant_get_next_user_type());
                    logger_register_service_with_id(service->service_id, service->service_name);

                    cli_add_logging_class(service->service_name);
                    cli_add_service(service->service_name);

                    //stack_push_back(service_list, variant_create_ptr(DT_PTR, service, NULL));

                    uint32_t key = crc32(0, service->service_name, strlen(service->service_name));
                    variant_hash_insert(service_table, key, variant_create_ptr(DT_PTR, service, NULL));

                    *(void**) (&service_cli_create) = dlsym(handle, "service_cli_create");
                    char* error = dlerror();
                    if(NULL == error)
                    {
                        (*service_cli_create)(root_node);
                    }
                }
            }

            memset(full_path, 0, sizeof(full_path));
        }

        closedir(dp);
        LOG_ADVANCED(ServiceManager, "Service manager initialized with %d services", service_table->count);

        event_register_handler(ServiceManager, COMMAND_ACTIVATION_EVENT, service_manager_on_command_activation_event, NULL);
    }
    else
    {
        LOG_ERROR(ServiceManager, "Error initializing service manager: Couldn't open the directory");
    }
}

/*variant_t*  service_manager_eval_method(service_method_t* method, ...)
{
    variant_t*  retVal = NULL;
    va_list args;
    va_start(args, method);

    return method->eval_callback(method, args);
}*/

service_method_t*  service_manager_get_method(const char* service_class, const char* name)
{
    service_t* service = service_manager_get_class(service_class);

    if(NULL != service)
    {
        stack_for_each(service->service_methods, method_variant)
        {
            service_method_t* method = variant_get_ptr(method_variant);
            if(strcmp(method->name, name) == 0)
            {
                return method;
            }
        }
    }

    return NULL;
}

service_t*  service_manager_get_class(const char* service_class)
{
    uint32_t key = crc32(0, service_class, strlen(service_class));
    variant_t* retVal = variant_hash_get(service_table, key);

    return (NULL == retVal)? NULL : (service_t*)variant_get_ptr(retVal);
}

service_t*  service_manager_get_class_by_id(int service_id)
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

bool    service_manager_is_class_exists(const char* service_class)
{
    uint32_t key = crc32(0, service_class, strlen(service_class));
    return variant_hash_get(service_table, key) != NULL;
}

void    service_manager_for_each_class(void (*visitor)(service_t*, void*), void* arg)
{
    variant_hash_for_each_value(service_table, service_t*, visitor, arg)
}

void    service_manager_for_each_method(const char* service_class, void (*visitor)(service_method_t*, void*), void* arg)
{
    service_t* service = service_manager_get_class(service_class);

    if(NULL != service)
    {
        stack_for_each(service->service_methods, method_variant)
        {
            service_method_t* method = (service_method_t*)variant_get_ptr(method_variant);
            visitor(method, arg);
        }
    }
}

void service_manager_on_command_activation_event(event_t* event, void* context)
{
    const char* cmd = variant_get_string(event->data);

    if(NULL != cmd)
    {
        LOG_ADVANCED(ServiceManager, "Command activation event from id %d with command %s", event->source_id, cmd);
        bool isOk = true;
        variant_stack_t* compiled = command_parser_compile_expression(cmd, &isOk);
    
        if(isOk)
        {
            variant_t* result = command_parser_execute_expression(compiled);
    
            if(NULL != result)
            {
                LOG_ADVANCED(ServiceManager, "Command %s executed successfully", cmd);
                variant_free(result);
            }
            else
            {
                LOG_ERROR(ServiceManager, "Failed to execute command %s", cmd);
            }
        }
        else
        {
            LOG_ERROR(ServiceManager, "Error parsing command %s", cmd);
        }
    }
    else
    {
        LOG_ERROR(ServiceManager, "Empty command");
    }
}

/**
 * Still dont know if this method is needed.... 
 * Is that a good idea to forward events to services???? 
 * 
 * @author alex (2/18/2016)
 * 
 * @param event 
 */
/*void    service_manager_on_event(event_t* event)
{
    switch(event->data->type)
    {
    case DT_SENSOR_EVENT_DATA:
        {
            // Forward sensor event to all services
            hash_iterator_t* it = variant_hash_begin(service_table);
    
            while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
            {
                service_t* service = (service_t*)variant_get_ptr(variant_hash_iterator_value(it));
                stack_for_each(service->event_subscriptions, event_subscription_variant)
                {
                    event_subscription_t* es = VARIANT_GET_PTR(event_subscription_t, event_subscription_variant);
                    if(strcmp(es->source, SENSOR_EVENT_SOURCE) == 0)
                    {
                        sensor_event_data_t* event_data = (sensor_event_data_t*)event->data;
                        LOG_DEBUG(ServiceManager, "Forward sensor event from: %s to service: %s", event_data->device_name, service->service_name);
                        es->on_event(SENSOR_EVENT_SOURCE, event);
                    }
                }
            }

            free(it);
        }
        break;
    case DT_SERVICE_EVENT_DATA:
        {
            service_t* calling_service = service_manager_get_class_by_id(event->source_id);
    
            if(NULL != calling_service)
            {
                hash_iterator_t* it = variant_hash_begin(service_table);
    
                while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
                {
                    service_t* service = (service_t*)variant_get_ptr(variant_hash_iterator_value(it));
            
                    stack_for_each(service->event_subscriptions, event_subscription_variant)
                    {
                        event_subscription_t* es = VARIANT_GET_PTR(event_subscription_t, event_subscription_variant);
                        if(strcmp(es->source, calling_service->service_name) == 0)
                        {
                            LOG_DEBUG(ServiceManager, "Forward event from: %s to service: %s", calling_service->service_name, service->service_name);
                            es->on_event(calling_service->service_name, event);
                        }
                    }
                }

                free(it);
            }
        }
        break;
    default:
        break;
    }
}*/
