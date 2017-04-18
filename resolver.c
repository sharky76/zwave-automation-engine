#include "resolver.h"
#include <logger.h>
#include "variant_types.h"
#include <hash.h>
#include <crc32.h>

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
    hash_table_t*  record_table;
} resolver_handle;


static resolver_handle* resolver;
DECLARE_LOGGER(Resolver)

void   resolver_init()
{
    LOG_ADVANCED(Resolver, "Initializing resolver...");

    resolver = (resolver_handle*)malloc(sizeof(resolver_handle));
    resolver->record_table = variant_hash_init();


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

    hash_iterator_t* it = variant_hash_begin(resolver->record_table);

    while(!variant_hash_iterator_is_end(variant_hash_iterator_next(it)))
    {
        device_record_t* record = (device_record_t*)variant_get_ptr(variant_hash_iterator_value(it));
        if(record->nodeId == nodeId && 
           record->instanceId == instanceId && 
           record->commandId == commandId)
        {
            name = record->deviceName;
            break;
        }

        //it = variant_hash_iterator_next(it);
    }

    free(it);

    return name;
}

const char* resolver_name_from_node_id(ZWBYTE nodeId)
{
    return resolver_name_from_id(nodeId, -1, -1);
}

bool resolver_has_name(const char* name)
{
    return (resolver_get_device_record(name) != NULL);
}

device_record_t*    resolver_get_device_record(const char* name)
{
    uint32_t key = crc32(0, name, strlen(name));

    variant_t* record_variant = variant_hash_get(resolver->record_table, key);
    if(NULL != record_variant)
    {
        return (device_record_t*)variant_get_ptr(record_variant);
    }

    return NULL;
}

void    resolver_add_entry(DeviceType devtype, const char* name, int nodeId, ZWBYTE instanceId, ZWBYTE commandId)
{
    device_record_t* record = resolver_get_device_record(name);
    if(record == NULL)
    {
        device_record_t* new_record = (device_record_t*)calloc(1, sizeof(device_record_t));
        new_record->devtype = devtype;
        new_record->nodeId = nodeId;
        new_record->instanceId = instanceId;
        new_record->commandId = commandId;
        strncpy(new_record->deviceName, name, MAX_DEVICE_NAME_LEN-1);

        uint32_t key = crc32(0, new_record->deviceName, strlen(new_record->deviceName));

        //stack_push_back(resolver->record_list, variant_create_ptr(DT_PTR, new_record, variant_delete_default));
        variant_hash_insert(resolver->record_table, key, variant_create_ptr(DT_PTR, new_record, variant_delete_default));
    }
    else 
    {
        resolver_remove_entry(name);
        resolver_add_entry(devtype, name, nodeId, instanceId, commandId);
    }
}

void resolver_add_device_entry(DeviceType devtype, const char* name, int nodeId)
{
    resolver_add_entry(devtype,name,nodeId,-1, -1);
}

void resolver_remove_entry(const char* name)
{
    uint32_t key = crc32(0, name, strlen(name));
    variant_hash_remove(resolver->record_table, key);
}

typedef struct
{
    void (*resolver_visitor)(device_record_t*, void*);
    void* resovler_arg;
} resolver_visitor_data_t;

void call_resolver_visitor(hash_node_data_t* hash_node, void* arg)
{
    resolver_visitor_data_t* resovler_data = (resolver_visitor_data_t*)arg;
    device_record_t* record = (device_record_t*)variant_get_ptr(hash_node->data);

    resovler_data->resolver_visitor(record, resovler_data->resovler_arg);
}

void    resolver_for_each(void (*visitor)(device_record_t*, void*), void* arg)
{
    resolver_visitor_data_t resolver_data = {
        .resolver_visitor = visitor,
        .resovler_arg = arg
    };

    variant_hash_for_each(resolver->record_table, call_resolver_visitor, &resolver_data);
}

