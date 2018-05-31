#ifndef RESOLVER_H
#define RESOLVER_H

#include <stdbool.h>
#include "vdev.h"
#include "stack.h"

// This is the device resolver module
// It uses json-c lib to load resolver.json file containing 
// node_id to name mappings

//#include <json-c/json.h>
#include "stack.h"

#define MAX_DEVICE_NAME_LEN 128
#define INVALID_INSTANCE -1

typedef struct device_record_t
{
    DeviceType devtype;
    char    deviceName[MAX_DEVICE_NAME_LEN];
    int  nodeId;
    int  instanceId;
    int  commandId;
    struct device_record_t* parent;
} device_record_t;

//#define resolver_for_each(_resolver_, _device_record_)  \
//device_record_t*    _device_record_ = &(_resolver_->device_record_list[0]);    \
//for(int _device_record_##i = 0; _device_record_##i < _resolver_->size; _device_record_##i++, _device_record_ = &(_resolver_->device_record_list[_device_record_##i]))

void                    resolver_init();
void                    resolver_add_entry(DeviceType devtype, const char* name, int nodeId, int instanceId, int commandId);
void                    resolver_add_device_entry(DeviceType devtype, const char* name, int nodeId);
void                    resolver_remove_entry(const char* name);
bool                    resolver_has_name(const char* name);
const char*             resolver_name_from_id(int nodeId, int instanceId, int commandId);
const char*             resolver_name_from_node_id(int nodeId);
device_record_t*        resolver_get_device_record(const char* name);
device_record_t*        resolver_resolve_id(int nodeId, int instanceId, int commandId);
void                    resolver_for_each(void (*visitor)(device_record_t*, void*), void* arg);
variant_stack_t*        resolver_get_instances(int node_id);

#endif // RESOLVER_H
