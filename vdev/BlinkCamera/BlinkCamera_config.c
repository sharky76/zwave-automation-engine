#include "BlinkCamera_config.h"
#include "resolver.h"
#include <stdlib.h>

void delete_blink_camera_entry(void* arg)
{
    blink_camera_entry_t* entry = (blink_camera_entry_t*)arg;
    free(entry->name);
    free(entry);
}

blink_camera_entry_t*    blink_camera_add(device_record_t* record)
{
    blink_camera_entry_t* entry = malloc(sizeof(blink_camera_entry_t));
    entry->name = strdup(record->deviceName);
    entry->instance = record->instanceId;
    entry->active_event_tick_counter = 0;
    entry->camera_motion_detected_event = false;
    
    stack_push_back(blink_camera_list, variant_create_ptr(DT_PTR, entry, &delete_blink_camera_entry));
    return entry;
}

blink_camera_entry_t*    blink_camera_find_instance(int instance)
{
    stack_for_each(blink_camera_list, camera_entry_variant)
    {
        blink_camera_entry_t* entry = VARIANT_GET_PTR(blink_camera_entry_t, camera_entry_variant);
        if(entry->instance == instance)
        {
            return entry;
        }
    }

    return NULL;
}

blink_camera_entry_t*    blink_camera_find_name(const char* name)
{
    if(NULL != name)
    {
        stack_for_each(blink_camera_list, camera_entry_variant)
        {
            blink_camera_entry_t* entry = VARIANT_GET_PTR(blink_camera_entry_t, camera_entry_variant);
            if(strcmp(entry->name, name) == 0)
            {
                return entry;
            }
        }
    }

    return NULL;
}
