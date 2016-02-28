#include "resolver.h"
#include <logger.h>
#include "variant_types.h"

/* Resolver examples:
 
  {
   "resolver": [
      {"nodeId": 1, "name":   "RazBerry Controller"},
      {"nodeId": 2, "name":   "Patio Door Sensor"}
    ]
} 
 
*/
typedef struct resolver_handle_t
{
    variant_stack_t*  record_list;
} resolver_handle;


static resolver_handle* resolver;
DECLARE_LOGGER(Resolver)

void   resolver_init()
{
    LOG_ADVANCED(Resolver, "Initializing resolver...");

    resolver = (resolver_handle*)malloc(sizeof(resolver_handle));
    resolver->record_list = stack_create();


/*
    struct json_object* root_object = json_object_from_file("resolver.json");
    if(NULL != root_object)
    {
        //resolver_handle* handle = (resolver_handle*)malloc(sizeof(resolver_handle));
        //memset(handle->device_record_list, 0, sizeof(handle->device_record_list));

        struct json_object* value;
        if(json_object_object_get_ex(root_object, "resolver", &value))
        {
            int size = json_object_array_length(value);

            int i;
            for(i = 0; i < size; i++)
            {
                struct json_object* record = json_object_array_get_idx(value, i);
                json_object_object_foreach(record, key, val)
                {
                    switch(json_object_get_type(val))
                    {
                    case json_type_int:
                        LOG_DEBUG("Key: %s, val: %d", key, json_object_get_int(val));
                        handle->device_record_list[i].nodeId = json_object_get_int(val);
                        break;
                    case json_type_string:
                        LOG_DEBUG("Key: %s, val: %s", key, json_object_get_string(val));
                        strncpy(handle->device_record_list[i].deviceName, json_object_get_string(val), MAX_DEVICE_NAME_LEN-1);
                        break;
                    }
                }
            }

            LOG_ADVANCED("Resolver initialized with %d entries", handle->size);
            return handle;
        }
    }

    LOG_ERROR("Error initializing resolver");
    return NULL;
*/
}

const char* resolver_name_from_id(ZWBYTE nodeId, ZWBYTE instanceId, ZWBYTE commandId)
{
    char* name = NULL;

    stack_for_each(resolver->record_list, dev_record_variant)
    {
        device_record_t* record = (device_record_t*)variant_get_ptr(dev_record_variant);
        if(record->nodeId == nodeId && 
           record->instanceId == instanceId && 
           record->commandId == commandId)
        {
            name = record->deviceName;
            break;
        }
    }

    return name;
}

bool resolver_has_name(const char* name)
{
    return (resolver_get_device_record(name) != NULL);
}

device_record_t*    resolver_get_device_record(const char* name)
{
    stack_for_each(resolver->record_list, dev_record_variant)
    {
        device_record_t* record = (device_record_t*)variant_get_ptr(dev_record_variant);
        if(strncmp(record->deviceName, name, MAX_DEVICE_NAME_LEN) == 0)
        {
            return record;
        }
    }

    return NULL;
}

void    resolver_add_entry(const char* name, ZWBYTE nodeId, ZWBYTE instanceId, ZWBYTE commandId)
{
    device_record_t* record = resolver_get_device_record(name);
    if(record == NULL)
    {
        device_record_t* new_record = (device_record_t*)calloc(1, sizeof(device_record_t));
        new_record->nodeId = nodeId;
        new_record->instanceId = instanceId;
        new_record->commandId = commandId;
        strncpy(new_record->deviceName, name, MAX_DEVICE_NAME_LEN-1);
        stack_push_back(resolver->record_list, variant_create_ptr(DT_DEV_RECORD, new_record, variant_delete_default));
    }
    else 
    {
        resolver_remove_entry(name);
        resolver_add_entry(name, nodeId, instanceId, commandId);
    }
}

void resolver_remove_entry(const char* name)
{
    stack_for_each(resolver->record_list, dev_record_variant)
    {
        device_record_t* query = (device_record_t*)variant_get_ptr(dev_record_variant);
        if(strcmp(query->deviceName, name) == 0)
        {
            stack_remove(resolver->record_list, dev_record_variant);
            variant_free(dev_record_variant);
            break;
        }
    }
}

void    resolver_for_each(void (*visitor)(device_record_t*, void*), void* arg)
{
    stack_for_each(resolver->record_list, dev_record_variant)
    {
        device_record_t* record = (device_record_t*)variant_get_ptr(dev_record_variant);
        visitor(record, arg);
    }
}

