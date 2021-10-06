#include "Sensor_config.h"
#include <resolver.h>
#include <logger.h>

void delete_sensor_entry(void* arg)
{
    sensor_entry_t* entry = (sensor_entry_t*)arg;
    free(entry->name);
    free(entry);
}

sensor_entry_t*    sensor_add(int instance, const char* name)
{
    sensor_entry_t* entry = malloc(sizeof(sensor_entry_t));
    entry->name = strdup(name);
    entry->instance = instance;
    entry->state = IDLE;
    
    stack_push_back(sensor_list, variant_create_ptr(DT_PTR, entry, &delete_sensor_entry));
    char name_buf[32] = {0};
    snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x30);
    resolver_add_entry(VDEV, name_buf, DT_SENSOR, entry->instance, 0x30);

    memset(name_buf, 0, sizeof(name_buf));
    snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x72);
    resolver_add_entry(VDEV, name_buf, DT_SENSOR, entry->instance, 0x72);
    LOG_ADVANCED(DT_SENSOR, "Added sensor %s", entry->name);

    return entry;
}

bool  sensor_del(int instance)
{
    variant_t* victim = NULL;

    stack_for_each(sensor_list, sensor_variant)
    {
        sensor_entry_t* entry = VARIANT_GET_PTR(sensor_entry_t, sensor_variant);
        if(entry->instance == instance)
        {
            victim = sensor_variant;
            char name_buf[32] = {0};
            snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x30);
            resolver_remove_entry(name_buf);
            memset(name_buf, 0, sizeof(name_buf));
            snprintf(name_buf, 31, "%s.%d.%d", entry->name, entry->instance, 0x30);
            resolver_remove_entry(name_buf);
            LOG_ADVANCED(DT_SENSOR, "Removed sensor %s", entry->name);
            break;
        }
    }

    if(NULL != victim)
    {
        stack_remove(sensor_list, victim);
        variant_free(victim);
    }

    return true;
}

sensor_entry_t*    sensor_find_instance(int instance)
{
    stack_for_each(sensor_list, sensor_variant)
    {
        sensor_entry_t* entry = VARIANT_GET_PTR(sensor_entry_t, sensor_variant);
        if(entry->instance == instance)
        {
            return entry;
        }
    }

    return NULL;
}
