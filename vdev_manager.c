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
#include <event_log.h>
#include <event_dispatcher.h>
#include <event_pump.h>

DECLARE_LOGGER(VDevManager)

hash_table_t* vdev_table;
command_class_t*    vdev_manager_create_command_class(vdev_t* vdev);
variant_t*   vdev_call_method(const char* method, device_record_t* vdev, va_list args);
static int vdev_id_counter = 260;

static int vdev_get_next_user_type()
{
    return vdev_id_counter++;
}

void    vdev_manager_init(const char* vdev_dir)
{
    LOG_ADVANCED(VDevManager, "Initializing virtual device manager...");
    DIR *dp;
    struct dirent *ep;     
    dp = opendir (vdev_dir);

    void    (*vdev_create)(vdev_t**);
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
                    //printf("VDEV_ID = %d\n", vdev_id);
                    (*vdev_create)(&vdev);

                    if(NULL != variant_hash_get(vdev_table, vdev->vdev_id))
                    {
                        LOG_ERROR(VDevManager, "Duplicate ID %d", vdev->vdev_id);
                        continue;
                    }
                    
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

                    // add resolver entry
                    resolver_add_device_entry(VDEV, vdev->name, vdev->vdev_id);

                    // Create command class pointer 
                    vdev->command_class_ptr = vdev_manager_create_command_class(vdev);
                }
            }

            memset(full_path, 0, sizeof(full_path));
        }

        closedir(dp);
        LOG_ADVANCED(VDevManager, "Virtual device manager initialized with %d devices", vdev_table->count);

        //event_register_handler(vdev_manager_on_event);
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
        
        event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
        event_pump_send_event(pump, DeviceAddedEvent, (void*)(int)vdev->vdev_id, NULL);
    }

    free(it);
}

command_class_t* vdev_manager_get_command_class(int vdev_id)
{
    vdev_t* vdev = (vdev_t*)variant_get_ptr(variant_hash_get(vdev_table, vdev_id));

    if(NULL != vdev)
    {
        return (command_class_t*)vdev->command_class_ptr;
    }

    return NULL;
}

vdev_t* vdev_manager_get_vdev_by_id(int vdev_id)
{
    return (vdev_t*)variant_get_ptr(variant_hash_get(vdev_table, vdev_id));
}

command_class_t*    vdev_manager_create_command_class(vdev_t* vdev)
{
    command_class_t* cmd_class = calloc(1, sizeof(command_class_t));
    cmd_class->command_id = vdev->vdev_id;
    cmd_class->command_name = vdev->name;
    cmd_class->command_impl = vdev_call_method; 
     

    int index = 0;
    stack_for_each(vdev->supported_method_list, method_variant)
    {
        vdev_command_t* cmd = (vdev_command_t*)variant_get_ptr(method_variant);
        cmd_class->supported_method_list[index].name = cmd->name;
        cmd_class->supported_method_list[index].nargs = cmd->nargs;

        index++;
        if(index >= 20)
        {
            break;
        }
    }
    return cmd_class;
}

device_record_t*    vdev_manager_create_device_record(const char* vdev_name)
{
    vdev_t* vdev = vdev_manager_get_vdev(vdev_name);
    device_record_t* record = NULL;

    if(NULL != vdev)
    {
        record = calloc(1, sizeof(device_record_t));
        strncpy(record->deviceName, vdev->name, 127);
        record->nodeId = vdev->vdev_id;
        record->devtype = VDEV;
    }

    return record;
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
    if(NULL == vdev)
    {
        LOG_ERROR(VDevManager, "Unable to call method %s on VDEV id %d: device not found", method, vdev_record->nodeId);
        return NULL;
    }

    stack_for_each(vdev->supported_method_list, vdev_method_variant)
    {
        vdev_command_t* cmd = (vdev_command_t*)variant_get_ptr(vdev_method_variant);
        // If command is defined as COMMAND_CLASS (VDEV_ADD_COMMAND_CLASS) we allow multiple 
        // commands with the same name. In that case cmd->command_id must be non-null and unique
        // for this command

        if(strcmp(cmd->name, method) == 0 && 
            ((!cmd->is_command_class && vdev_record->commandId == INVALID_INSTANCE) || 
             (cmd->is_command_class && cmd->command_id == vdev_record->commandId)))
        {
            return cmd->command_impl(vdev_record, args);
        }
    }
    
    return NULL;
}

