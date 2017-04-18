#ifndef RESOLVER_H
#define RESOLVER_H

#include <stdbool.h>

// This is the device resolver module
// It uses json-c lib to load resolver.json file containing 
// node_id to name mappings

//#include <json-c/json.h>
#include <ZPlatform.h>
#include <stack.h>

#define MAX_DEVICE_NAME_LEN 128

typedef enum DeviceType
{
    ZWAVE,
    VDEV
} DeviceType;

typedef struct device_record_t
{
    DeviceType devtype;
    char    deviceName[MAX_DEVICE_NAME_LEN];
    int  nodeId;
    ZWBYTE  instanceId;
    ZWBYTE  commandId;
} device_record_t;

//#define resolver_for_each(_resolver_, _device_record_)  \
//device_record_t*    _device_record_ = &(_resolver_->device_record_list[0]);    \
//for(int _device_record_##i = 0; _device_record_##i < _resolver_->size; _device_record_##i++, _device_record_ = &(_resolver_->device_record_list[_device_record_##i]))

void                    resolver_init();
void                    resolver_add_entry(DeviceType devtype, const char* name, int nodeId, ZWBYTE instanceId, ZWBYTE commandId);
void                    resolver_add_device_entry(DeviceType devtype, const char* name, int nodeId);
void                    resolver_remove_entry(const char* name);
bool                    resolver_has_name(const char* name);
const char*             resolver_name_from_id(ZWBYTE nodeId, ZWBYTE instanceId, ZWBYTE commandId);
const char*             resolver_name_from_node_id(ZWBYTE nodeId);
device_record_t*        resolver_get_device_record(const char* name);
void                    resolver_for_each(void (*visitor)(device_record_t*, void*), void* arg);

#endif // RESOLVER_H
