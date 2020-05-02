#include "BlinkCamera_config.h"
#include "resolver.h"
#include <stdlib.h>
#include <logger.h>

void delete_blink_camera_entry(void* arg)
{
    blink_camera_entry_t* entry = (blink_camera_entry_t*)arg;
    free(entry->name);
    free(entry);
}

blink_camera_entry_t*    blink_camera_add(int instance, const char* name)
{
    blink_camera_entry_t* entry = malloc(sizeof(blink_camera_entry_t));
    entry->name = strdup(name);
    entry->instance = instance;
    entry->active_event_tick_counter = 0;
    entry->camera_motion_detected_event = false;
    
    stack_push_back(blink_camera_list, variant_create_ptr(DT_PTR, entry, &delete_blink_camera_entry));

    char name_buf[32] = {0};
    snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 48);
    resolver_add_entry(VDEV, name_buf, DT_BLINK_CAMERA, entry->instance, 48);

    memset(name_buf, 0, sizeof(name_buf));
    snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x72);
    resolver_add_entry(VDEV, name_buf, DT_BLINK_CAMERA, entry->instance, 0x72);
    LOG_ADVANCED(DT_BLINK_CAMERA, "Added button %s", entry->name);

    return entry;
}

bool    blink_camera_del(int instance)
{
    variant_t* victim = NULL;

    stack_for_each(blink_camera_list, camera_variant)
    {
        blink_camera_entry_t* entry = VARIANT_GET_PTR(blink_camera_entry_t, camera_variant);
        if(entry->instance == instance)
        {
            victim = camera_variant;
            char name_buf[32] = {0};
            snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 48);
            resolver_remove_entry(name_buf);
            memset(name_buf, 0, sizeof(name_buf));
            snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x72);
            resolver_remove_entry(name_buf);
            LOG_ADVANCED(DT_BLINK_CAMERA, "Removed button %s", entry->name);
            break;
        }
    }

    if(NULL != victim)
    {
        stack_remove(blink_camera_list, victim);
        variant_free(victim);
    }

    return true;
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
