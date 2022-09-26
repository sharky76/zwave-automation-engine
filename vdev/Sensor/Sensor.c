#include "Sensor_config.h"
#include "Sensor_cli.h"
#include <vdev.h>
#include <stdio.h>
#include <logger.h>
#include <crc32.h>
#include <event.h>
#include <event_log.h>
#include <resolver.h>

int  DT_SENSOR = 264;

void        device_start(); 
variant_t*  get_model_info(device_record_t* record, va_list args);
variant_t*  get_sensor_state(device_record_t* record, va_list args);
variant_t*  set_sensor_state(device_record_t* record, va_list args);
void  publish_sensor_state(sensor_entry_t* entry);

void    vdev_create(vdev_t** vdev)
{
    VDEV_INIT(DT_SENSOR, "Sensor", device_start)

    VDEV_ADD_COMMAND_CLASS_GET("Get", 0x30, "1.level", 0, get_sensor_state, "Get state of Sensor")
    VDEV_ADD_COMMAND_CLASS_SET("Set", 0x30, "1.level", 1, set_sensor_state, "Set state of Sensor")
    VDEV_ADD_COMMAND_CLASS_GET("Get", 0x72, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_CONFIG_PROVIDER(Sensor_get_config);

    sensor_list = stack_create();

    command_parser_register_symbol("ALERTED", variant_create_byte(DT_INT8, ALERTED));
    command_parser_register_symbol("IDLE", variant_create_byte(DT_INT8, IDLE));
}

void    device_start()
{
    LOG_INFO(DT_SENSOR, "Starting Sensor device");

    if(sensor_list->count == 0)
    {
        LOG_INFO(DT_SENSOR, "No configured sensors found");
    }

    LOG_INFO(DT_SENSOR, "Sensor device started");
}

void    vdev_cli_create(cli_node_t* parent_node)
{
    Sensor_cli_create(parent_node);
}

variant_t*  get_model_info(device_record_t* record, va_list args)
{
    char* model_info = "{\"vendor\":\"Alex\", \"model\":\"Sensor\"}";
    return variant_create_string(strdup(model_info));
}

variant_t*  get_sensor_state(device_record_t* record, va_list args)
{
    sensor_entry_t* entry = sensor_find_instance(record->instanceId);
    if(NULL != entry)
    {
        LOG_DEBUG(DT_SENSOR, "Sensor instance %d status %d", record->instanceId, entry->state);
        return variant_create_bool((entry->state == IDLE)? false : true);
    }
    else
    {
        LOG_ERROR(DT_SENSOR, "Sensor instance %d not found", record->instanceId);
        return variant_create_bool(false);
    }
}

variant_t*  set_sensor_state(device_record_t* record, va_list args)
{
    variant_t* new_state_variant = va_arg(args, variant_t*);
    bool new_state = variant_get_bool(new_state_variant);

    sensor_entry_t* entry = sensor_find_instance(record->instanceId);
    if(NULL != entry)
    {
        LOG_DEBUG(DT_SENSOR, "Sensor instance %d set state %d", record->instanceId, new_state);
        entry->state = (new_state)? ALERTED : IDLE;
        publish_sensor_state(entry);
        vdev_post_event(VdevDataChangeEvent, DT_SENSOR, 0x30, record->instanceId);
    }
    else
    {
        LOG_ERROR(DT_SENSOR, "Sensor instance %d not found", record->instanceId);
        return variant_create_bool(false);
    }

    return variant_create_bool(true);
}

void  publish_sensor_state(sensor_entry_t* entry)
{
    if(NULL != entry)
    {
        LOG_INFO(DT_SENSOR, "Sensor %s state set to %d", entry->name, entry->state);
        char json_buf[256] = {0};
        
        snprintf(json_buf, 255, "{\"data_holder\":\"1\",\"level\":%d}",
                entry->state);

        event_entry_t* new_entry = event_create();
        new_entry->node_id = DT_SENSOR;
        new_entry->instance_id = entry->instance;
        new_entry->command_id = 0x30;
        new_entry->device_type = VDEV;
        new_entry->event_data = strdup(json_buf);
        event_send(new_entry);
    }
    else
    {
        LOG_ERROR(DT_SENSOR, "Sensor instance %d not found", entry->instance);
    }
}
