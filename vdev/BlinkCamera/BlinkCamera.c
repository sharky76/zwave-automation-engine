#include <vdev.h>
#include <stdio.h>
#include <logger.h>
#include <crc32.h>
#include <event.h>
#include <event_log.h>
#include <resolver.h>
#include "service.h"
#include "service_args.h"
#include "BlinkCamera_config.h"
#include "BlinkCamera_cli.h"

#define EVENT_ACTIVE_TIMEOUT_SEC 180

service_method_t* builtin_service_manager_get_method(const char* service_class, const char* method_name);

void        device_start(); 
variant_t*  get_model_info(device_record_t* record, va_list args);
variant_t*  get_all_motion_events(device_record_t* record, va_list args);
variant_t*  set_camera_motion_detected(device_record_t* record, va_list args);
variant_t*  set_camera_armed(device_record_t* record, va_list args);

void        timer_tick_handler(event_t* pevent, void* context);
void        on_motion_timeout(event_pump_t* pump, int handler_id, void* context);

void    vdev_create(vdev_t** vdev)
{
    DT_BLINK_CAMERA = 261;
    VDEV_INIT(DT_BLINK_CAMERA, "BlinkCamera", device_start)

    VDEV_ADD_COMMAND_CLASS_GET("Get", 48, "1.level", 0, get_all_motion_events, "Get Motion event count from all cameras")
    VDEV_ADD_COMMAND_CLASS_GET("Get", 114, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_COMMAND("MotionDetected", 2, set_camera_motion_detected, "Set camera motion detected event (args: camera name, bool)")
    VDEV_ADD_COMMAND("SetArmed", 1, set_camera_armed, "Arm/Disarm camera motion detection")
    VDEV_ADD_CONFIG_PROVIDER(BlinkCamera_get_config);

    blink_camera_list = stack_create();
}

void        device_start()
{
    LOG_INFO(DT_BLINK_CAMERA, "Starting Blink Camera device");

    if(blink_camera_list->count == 0)
    {
        LOG_INFO(DT_BLINK_CAMERA, "No configured vameras found");
    }

    LOG_INFO(DT_BLINK_CAMERA, "Blink Camera device started");
}

void    vdev_cli_create(cli_node_t* parent_node)
{
    BlinkCamera_cli_create(parent_node);
}

variant_t*  get_model_info(device_record_t* record, va_list args)
{
    char* model_info = "{\"vendor\":\"Alex\", \"model\":\"Blink Camera\"}";
    return variant_create_string(strdup(model_info));
}

variant_t*  get_all_motion_events(device_record_t* record, va_list args)
{
    LOG_DEBUG(DT_BLINK_CAMERA, "Getting motion info for Camera instance: %d", record->instanceId);
    blink_camera_entry_t* entry = blink_camera_find_instance(record->instanceId);
    if(NULL != entry)
    {
        LOG_DEBUG(DT_BLINK_CAMERA, "Camera instance %d status %d", record->instanceId, entry->camera_motion_detected_event);
        return variant_create_bool(entry->camera_motion_detected_event);
    }
    else
    {
        LOG_ERROR(DT_BLINK_CAMERA, "Unable to get motion info, camera instance %d not found", record->instanceId);
        return variant_create_bool(false);
    }
}

void update_camera_motion_detected_state(int instance, bool state)
{
    blink_camera_entry_t* entry = blink_camera_find_instance(instance);

    if(NULL != entry)
    {
        entry->camera_motion_detected_event = state;
        entry->active_event_tick_counter = 0;

        LOG_INFO(DT_BLINK_CAMERA, "Blink Camera %s motion detection set to %d", entry->name, entry->camera_motion_detected_event);
        char json_buf[256] = {0};
    
        // Generate REST API event
        snprintf(json_buf, 255, "{\"data_holder\":\"1\",\"level\":%s}",
                 (entry->camera_motion_detected_event)? "true" : "false");
    
        event_log_entry_t* new_entry = calloc(1, sizeof(event_log_entry_t));
        new_entry->node_id = DT_BLINK_CAMERA;
        new_entry->instance_id = instance;
        new_entry->command_id = 48;
        new_entry->device_type = VDEV;
        new_entry->event_data = strdup(json_buf);
        event_log_add_event(new_entry);
    
        // Notify system about security state change
        vdev_post_event(VdevDataChangeEvent, DT_BLINK_CAMERA, 48, instance);
    }
}

variant_t*  set_camera_motion_detected(device_record_t* record, va_list args)
{
    variant_t* camera_name_variant = va_arg(args, variant_t*);
    variant_t* new_state_variant = va_arg(args, variant_t*);

    blink_camera_entry_t* entry = blink_camera_find_name(variant_get_string(camera_name_variant));

    if(NULL != entry)
    {
        if(true == variant_get_bool(new_state_variant))
        {
            LOG_DEBUG(DT_BLINK_CAMERA, "Starting motion threshold timer");
            event_pump_t* timer_pump = event_dispatcher_get_pump("TIMER_PUMP");
            timer_pump->start(timer_pump, entry->timer_id);
            update_camera_motion_detected_state(entry->instance, variant_get_bool(new_state_variant));
        }
        return variant_create_bool(true);
    }
    else
    {
        LOG_ERROR(DT_BLINK_CAMERA, "Unable to ename motion detection, no such camera %s", variant_get_string(camera_name_variant));
        return variant_create_bool(false);
    }
}

void on_motion_timeout(event_pump_t* pump, int handler_id, void* context)
{
    blink_camera_entry_t* entry = (blink_camera_entry_t*)context;
    LOG_DEBUG(DT_BLINK_CAMERA, "Reset active events for %s", entry->name);
    update_camera_motion_detected_state(entry->instance, false);
}

variant_t*  set_camera_armed(device_record_t* record, va_list args)
{
    variant_t* new_state_variant = va_arg(args, variant_t*);

    variant_t* ifttt_event;

    if(variant_get_bool(new_state_variant))
    {
        ifttt_event = variant_create_string(strdup("BlinkCameraArm"));
    }
    else
    {
        ifttt_event = variant_create_string(strdup("BlinkCameraDisarm"));
    }

    variant_t* ret = service_call_method("IFTTT", "Trigger", ifttt_event);
    variant_free(ifttt_event);

    return ret;
}
