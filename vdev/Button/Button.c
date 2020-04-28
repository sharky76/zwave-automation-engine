#include "Button_config.h"
#include <vdev.h>
#include <stdio.h>
#include <logger.h>
#include <crc32.h>
#include <event.h>
#include <event_log.h>
#include <resolver.h>

int  DT_BUTTON;

void        device_start(); 
variant_t*  get_model_info(device_record_t* record, va_list args);
variant_t*  get_button_state(device_record_t* record, va_list args);
variant_t*  set_button_state(device_record_t* record, va_list args);
variant_t*  update_button_state(device_record_t* record, va_list args);

void    vdev_create(vdev_t** vdev, int vdev_id)
{
    VDEV_INIT("Button", device_start)

    VDEV_ADD_COMMAND_CLASS("Get", 0x25, "level", 0, get_button_state, "Get state of Button")
    VDEV_ADD_COMMAND_CLASS("Set", 0x25, "level", 1, set_button_state, "Get state of Button")
    VDEV_ADD_COMMAND_CLASS("GetModelInfo", 0x72, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_COMMAND("ChangeState", 1, update_button_state, "Update the state of button");

    DT_BUTTON = vdev_id;
    button_list = stack_create();

    command_parser_register_symbol("ON", variant_create_byte(DT_INT8, ON));
    command_parser_register_symbol("OFF", variant_create_byte(DT_INT8, OFF));
}

void    device_start()
{
    LOG_INFO(DT_BUTTON, "Starting Button device");
    variant_stack_t* instance_list = resolver_get_instances(DT_BUTTON);

    if(instance_list->count == 0)
    {
        LOG_INFO(DT_BUTTON, "No configured buttons found");
    }

    stack_for_each(instance_list, device_record_variant)
    {
        device_record_t* record = VARIANT_GET_DEVICE_RECORD(device_record_variant);
        button_entry_t* entry = button_add(record);
        LOG_ADVANCED(DT_BUTTON, "Added button %s", entry->name);
    }

    LOG_INFO(DT_BUTTON, "Button device started");
}

variant_t*  get_model_info(device_record_t* record, va_list args)
{
    char* model_info = "{\"vendor\":\"Alex\"}";
    return variant_create_string(strdup(model_info));
}

variant_t*  get_button_state(device_record_t* record, va_list args)
{
    button_entry_t* entry = button_find_instance(record->instanceId);
    if(NULL != entry)
    {
        LOG_DEBUG(DT_BUTTON, "Button instance %d status %d", record->instanceId, entry->state);
        return variant_create_bool((entry->state == OFF)? false : true);
    }
    else
    {
        LOG_ERROR(DT_BUTTON, "Button instance %d not found", record->instanceId);
        return variant_create_bool(false);
    }
}

variant_t*  set_button_state(device_record_t* record, va_list args)
{
    variant_t* new_state_variant = va_arg(args, variant_t*);
    bool new_state = variant_get_bool(new_state_variant);

    button_entry_t* entry = button_find_instance(record->instanceId);
    if(NULL != entry)
    {
        LOG_DEBUG(DT_BUTTON, "Button instance %d set state %d", record->instanceId, new_state);
        entry->state = (new_state)? ON : OFF;
        update_button_state(record, args);
    }
    else
    {
        LOG_ERROR(DT_BUTTON, "Button instance %d not found", record->instanceId);
        return variant_create_bool(false);
    }
}

variant_t*  update_button_state(device_record_t* record, va_list args)
{
    variant_t* new_state_variant = va_arg(args, variant_t*);
    button_entry_t* entry = button_find_instance(record->instanceId);
    if(NULL != entry)
    {
        LOG_INFO(DT_BUTTON, "Button system state set to %d", entry->state);
        char json_buf[256] = {0};
        
        snprintf(json_buf, 255, "{\"data_holder\":\"level\",\"level\":%d}",
                entry->state);

        event_log_entry_t* new_entry = calloc(1, sizeof(event_log_entry_t));
        new_entry->node_id = DT_BUTTON;
        new_entry->instance_id = record->instanceId;
        new_entry->command_id = 0x25;
        new_entry->device_type = VDEV;
        new_entry->event_data = strdup(json_buf);
        event_log_add_event(new_entry);

        return variant_create_bool(true);
    }
    else
    {
        LOG_ERROR(DT_BUTTON, "Button instance %d not found", record->instanceId);
        return variant_create_bool(false);
    }
}
