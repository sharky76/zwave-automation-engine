#include "vdev_manager.h"
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include "logger.h"
#include <hash.h>
#include <cli.h>
#include "cli_logger.h"
#include <vdev.h>
#include "service_manager.h"
#include "cli_vdev.h"
#include <crc32.h>

DECLARE_LOGGER(VDevManager)

hash_table_t* vdev_table;
command_class_t*    vdev_manager_create_command_class(vdev_t* vdev, ZWBYTE command_id);
variant_t*   vdev_call_method(const char* method, device_record_t* vdev, va_list args);

void    vdev_manager_init(const char* vdev_dir)
{
    LOG_ADVANCED(VDevManager, "Initializing virtual device manager...");
    DIR *dp;
    struct dirent *ep;     
    dp = opendir (vdev_dir);

    void    (*vdev_create)(vdev_t**, int);
    void    (*vdev_cli_create)(cli_node_t* root_node);
    char full_path[256] = {0};
    
    vdev_table = variant_hash_init();

    if(dp != NULL)
    {
        while (ep = readdir (dp))
        {
            strcat(full_path, vdev_dir);
            strcat(full_path, "/");
            strcat(full_path, ep->d_name);

            void* handle = dlopen(full_path, RTLD_NOW);
            char* error = dlerror();
            if(NULL != error)
            {
                LOG_ERROR(VDevManager, "Error opening virtual device %s: %s", full_path, error);
            }

            if(NULL != handle)
            {
                LOG_DEBUG(VDevManager, "Loading virtual devices from %s", full_path);
                dlerror();
                *(void**) (&vdev_create) = dlsym(handle, "vdev_create");

                char* error = dlerror();
                if(NULL != error)
                {
                    LOG_ERROR(VDevManager, "Error getting virtual device entry point: %s", error);
                }
                else
                {
                    vdev_t* vdev;
                    (*vdev_create)(&vdev, variant_get_next_user_type());
                    logger_register_service_with_id(vdev->vdev_id, vdev->name);

                    cli_add_logging_class(vdev->name);

                    // Later...
                    cli_add_vdev(vdev->name);

                    //uint32_t key = crc32(0, vdev->name, strlen(vdev->name));
                    uint32_t key = vdev->vdev_id;
                    variant_hash_insert(vdev_table, key, variant_create_ptr(DT_PTR, vdev, NULL));

                    *(void**) (&vdev_cli_create) = dlsym(handle, "vdev_cli_create");
                    char* error = dlerror();
                    if(NULL == error)
                    {
                        (*vdev_cli_create)(root_node);
                    }

                    // Create command class pointer for each supported method
                    stack_for_each(vdev->supported_method_list, method_variant)
                    {
                        vdev_command_t* method = (vdev_command_t*)variant_get_ptr(method_variant);
                        method->command_class_ptr = vdev_manager_create_command_class(vdev, method->command_id);
                    }
                }
            }

            memset(full_path, 0, sizeof(full_path));
        }

        closedir(dp);
        LOG_ADVANCED(VDevManager, "Virtual device manager initialized with %d devices", vdev_table->count);

        event_register_handler(vdev_manager_on_event);
    }
    else
    {
        LOG_ERROR(VDevManager, "Error initializing virtual device manager: Couldn't open the directory");
    }
}

bool vdev_manager_is_vdev_exists(const char* vdev_name)
{
    vdev_t* vdev = vdev_manager_get_vdev(vdev_name);

    return (NULL != vdev);
}

vdev_t* vdev_manager_get_vdev(const char* vdev_name)
{
    hash_iterator_t* it = variant_hash_begin(vdev_table);
    while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
    {
        vdev_t* vdev = (vdev_t*)variant_get_ptr(variant_hash_iterator_value(it));
        if(strcmp(vdev->name, vdev_name) == 0)
        {
            free(it);
            return vdev;
        }
    }

    free(it);
    return NULL;
}

void vdev_manager_start_devices()
{
    LOG_INFO(VDevManager, "Starting virtual devices...");
    hash_iterator_t* it = variant_hash_begin(vdev_table);
    while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
    {
        vdev_t* vdev = (vdev_t*)variant_get_ptr(variant_hash_iterator_value(it));
        vdev->start();
    }

    free(it);
}

command_class_t* vdev_manager_get_command_class_by_id(int vdev_id, ZWBYTE command_id)
{
    vdev_t* vdev = (vdev_t*)variant_get_ptr(variant_hash_get(vdev_table, vdev_id));

    stack_for_each(vdev->supported_method_list, vdev_method_variant)
    {
        vdev_command_t* cmd = (vdev_command_t*)variant_get_ptr(vdev_method_variant);
        if(cmd->command_id == command_id)
        {
            return (command_class_t*)cmd->command_class_ptr;
        }
    }
    return NULL;
}

vdev_t* vdev_manager_get_vdev_by_id(int vdev_id)
{
    return (vdev_t*)variant_get_ptr(variant_hash_get(vdev_table, vdev_id));
}

command_class_t*    vdev_manager_create_command_class(vdev_t* vdev, ZWBYTE command_id)
{
    command_class_t* cmd_class = calloc(1, sizeof(command_class_t));
    cmd_class->command_id = command_id;
    cmd_class->command_name = vdev->name;
    cmd_class->command_impl = vdev_call_method; 
     

    int index = 0;
    stack_for_each(vdev->supported_method_list, method_variant)
    {
        vdev_command_t* cmd = (vdev_command_t*)variant_get_ptr(method_variant);
        cmd_class->supported_method_list[index].name = cmd->name;
        cmd_class->supported_method_list[index].nargs = cmd->nargs;

        index++;
        if(index >= 10)
        {
            break;
        }
    }
    return cmd_class;
}

void    vdev_manager_for_each(void (*visitor)(vdev_t*, void*), void* arg)
{
    variant_hash_for_each_value(vdev_table, vdev_t*, visitor, arg)
}

void    vdev_manager_for_each_method(const char* vdev_name, void (*visitor)(vdev_command_t*, void*), void* arg)
{
    vdev_t* vdev = vdev_manager_get_vdev(vdev_name);

    if(NULL != vdev)
    {
        stack_for_each(vdev->supported_method_list, method_variant)
        {
            vdev_command_t* method = (vdev_command_t*)variant_get_ptr(method_variant);
            visitor(method, arg);
        }
    }
}

variant_t*   vdev_call_method(const char* method, device_record_t* vdev_record, va_list args)
{
    vdev_t* vdev = (vdev_t*)variant_get_ptr(variant_hash_get(vdev_table, vdev_record->nodeId));

    stack_for_each(vdev->supported_method_list, vdev_method_variant)
    {
        vdev_command_t* cmd = (vdev_command_t*)variant_get_ptr(vdev_method_variant);
        if(strcmp(cmd->name, method) == 0)
        {
            return cmd->command_impl(args);
        }
    }
    
    return NULL;
}

void                vdev_manager_on_event(event_t* event)
{
    switch(event->data->type)
    {
    case DT_SENSOR_EVENT_DATA:
        {
            // Forward sensor event to all services
            hash_iterator_t* it = variant_hash_begin(vdev_table);
    
            while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
            {
                vdev_t* vdev = (vdev_t*)variant_get_ptr(variant_hash_iterator_value(it));
                stack_for_each(vdev->event_subscriptions, event_subscription_variant)
                {
                    event_subscription_t* es = VARIANT_GET_PTR(event_subscription_t, event_subscription_variant);
                    if(strcmp(es->source, SENSOR_EVENT_SOURCE) == 0)
                    {
                        sensor_event_data_t* event_data = (sensor_event_data_t*)event->data;
                        LOG_DEBUG(VDevManager, "Forward sensor event from: %s to virtual device: %s", event_data->device_name, vdev->name);
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
                hash_iterator_t* it = variant_hash_begin(vdev_table);
    
                while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
                {
                    vdev_t* vdev = (vdev_t*)variant_get_ptr(variant_hash_iterator_value(it));
            
                    stack_for_each(vdev->event_subscriptions, event_subscription_variant)
                    {
                        event_subscription_t* es = VARIANT_GET_PTR(event_subscription_t, event_subscription_variant);
                        if(strcmp(es->source, calling_service->service_name) == 0)
                        {
                            LOG_DEBUG(VDevManager, "Forward event from: %s to virtual device: %s", calling_service->service_name, vdev->name);
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
}
