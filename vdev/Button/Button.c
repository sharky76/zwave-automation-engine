#include "Button_config.h"
#include "Button_cli.h"
#include <vdev.h>
#include <stdio.h>
#include <logger.h>
#include <crc32.h>
#include <event.h>
#include <event_log.h>
#include <resolver.h>

int  DT_BUTTON = 263;

void        device_start(); 
variant_t*  get_model_info(device_record_t* record, va_list args);
variant_t*  get_button_state(device_record_t* record, va_list args);
variant_t*  set_button_state(device_record_t* record, va_list args);
void  publish_button_state(button_entry_t* entry);

void    vdev_create(vdev_t** vdev)
{
    VDEV_INIT(DT_BUTTON, "Button", device_start)

    VDEV_ADD_COMMAND_CLASS_GET("Get", 0x25, "level", 0, get_button_state, "Get state of Button")
    VDEV_ADD_COMMAND_CLASS_SET("Set", 0x25, "level", 1, set_button_state, "Set state of Button")
    VDEV_ADD_COMMAND_CLASS_GET("Get", 0x72, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_CONFIG_PROVIDER(Button_get_config);

    button_list = stack_create();

    command_parser_register_symbol("ON", variant_create_byte(DT_INT8, ON));
    command_parser_register_symbol("OFF", variant_create_byte(DT_INT8, OFF));
}

void    device_start()
{
    LOG_INFO(DT_BUTTON, "Starting Button device");

    if(button_list->count == 0)
    {
        LOG_INFO(DT_BUTTON, "No configured buttons found");
    }

    LOG_INFO(DT_BUTTON, "Button device started");
}

void    vdev_cli_create(cli_node_t* parent_node)
{
    Button_cli_create(parent_node);
}

variant_t*  get_model_info(device_record_t* record, va_list args)
{
    char* model_info = "{\"vendor\":\"Alex\", \"model\":\"Button\"}";
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
        publish_button_state(entry);
        vdev_post_event(VdevDataChangeEvent, DT_BUTTON, 0x25, record->instanceId);
    }
    else
    {
        LOG_ERROR(DT_BUTTON, "Button instance %d not found", record->instanceId);
        return variant_create_bool(false);
    }
}

void  publish_button_state(button_entry_t* entry)
{
    if(NULL != entry)
    {
        LOG_INFO(DT_BUTTON, "Button %s state set to %d", entry->name, entry->state);
        char json_buf[256] = {0};
        
        snprintf(json_buf, 255, "{\"data_holder\":\"level\",\"level\":%d}",
                entry->state);

        event_entry_t* new_entry = new_event_entry();
        new_entry->node_id = DT_BUTTON;
        new_entry->instance_id = entry->instance;
        new_entry->command_id = 0x25;
        new_entry->device_type = VDEV;
        new_entry->event_data = strdup(json_buf);
        event_send(new_entry, NULL);
    }
    else
    {
        LOG_ERROR(DT_BUTTON, "Button instance %d not found", entry->instance);
    }
}
