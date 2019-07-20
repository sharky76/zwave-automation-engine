#include "SS_config.h"
#include <vdev.h>
#include <stdio.h>
#include <logger.h>
#include <crc32.h>
#include <event.h>
#include <event_log.h>

int  DT_SECURITY_SYSTEM;
SecuritySystemState_t    SS_State;

void        device_start(); 
variant_t*  get_model_info(device_record_t* record, va_list args);
variant_t*  get_security_system_state(device_record_t* record, va_list args);
variant_t*  set_security_system_state(device_record_t* record, va_list args);
variant_t*  update_security_system_state(device_record_t* record, va_list args);

void    vdev_create(vdev_t** vdev, int vdev_id)
{
    VDEV_INIT("SecuritySystem", device_start)

    VDEV_ADD_COMMAND_CLASS("Get", COMMAND_CLASS_SECURITY_SYSTEM, "level", 0, get_security_system_state, "Get state of Security System")
    VDEV_ADD_COMMAND_CLASS("GetModelInfo", COMMAND_CLASS_MODEL_INFO, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_COMMAND("Set", 1, set_security_system_state, "Set state of Security System (0 - 4)")
    VDEV_ADD_COMMAND("ChangeState", 1, update_security_system_state, "Update the state of security system");

    DT_SECURITY_SYSTEM = vdev_id;
    SS_State = DISARMED;

    command_parser_register_symbol("STAY_ARMED", variant_create_byte(DT_INT8, STAY_ARMED));
    command_parser_register_symbol("AWAY_ARM", variant_create_byte(DT_INT8, AWAY_ARM));
    command_parser_register_symbol("NIGHT_ARM", variant_create_byte(DT_INT8, NIGHT_ARM));
    command_parser_register_symbol("DISARMED", variant_create_byte(DT_INT8, DISARMED));
    command_parser_register_symbol("ALARM_TRIGGERED", variant_create_byte(DT_INT8, ALARM_TRIGGERED));
}

void    device_start()
{
    LOG_INFO(DT_SECURITY_SYSTEM, "Starting Security System device");

    LOG_INFO(DT_SECURITY_SYSTEM, "Security System device started");
}

variant_t*  get_model_info(device_record_t* record, va_list args)
{
    char* model_info = "{\"vendor\":\"Alex\"}";
    return variant_create_string(strdup(model_info));
}

variant_t*  get_security_system_state(device_record_t* record, va_list args)
{
    return variant_create_byte(DT_INT8, SS_State);
}

variant_t*  set_security_system_state(device_record_t* record, va_list args)
{
    update_security_system_state(record, args);

    // Notify system about security state change
    vdev_post_event(VdevDataChangeEvent, DT_SECURITY_SYSTEM, COMMAND_CLASS_SECURITY_SYSTEM, 0);

    return variant_create_bool(true);
}

/**
 * Notify the world about state change
 * 
 * @author alex (5/2/2018)
 * 
 * @param event 
 * @param context 
 */
variant_t*  update_security_system_state(device_record_t* record, va_list args)
{
    variant_t* new_state_variant = va_arg(args, variant_t*);
    SS_State = variant_get_byte(new_state_variant);

    LOG_INFO(DT_SECURITY_SYSTEM, "Security system state set to %d", SS_State);
    char json_buf[256] = {0};
    
    snprintf(json_buf, 255, "{\"data_holder\":\"level\",\"level\":%d}",
             SS_State);

    event_log_entry_t* new_entry = calloc(1, sizeof(event_log_entry_t));
    new_entry->node_id = DT_SECURITY_SYSTEM;
    new_entry->instance_id = 0;
    new_entry->command_id = COMMAND_CLASS_SECURITY_SYSTEM;
    new_entry->device_type = VDEV;
    new_entry->event_data = strdup(json_buf);
    event_log_add_event(new_entry);

    return variant_create_bool(true);
}
