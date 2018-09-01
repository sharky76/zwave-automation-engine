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

#define EVENT_ACTIVE_TIMEOUT_SEC 180

service_method_t* builtin_service_manager_get_method(const char* service_class, const char* method_name);

int  DT_BLINK_CAMERA;

void        device_start(); 
variant_t*  get_model_info(device_record_t* record, va_list args);
variant_t*  get_all_motion_events(device_record_t* record, va_list args);
variant_t*  set_camera_motion_detected(device_record_t* record, va_list args);
variant_t*  set_camera_armed(device_record_t* record, va_list args);

void        timer_tick_handler(event_t* pevent, void* context);

void    vdev_create(vdev_t** vdev, int vdev_id)
{
    VDEV_INIT("BlinkCamera", device_start)

    VDEV_ADD_COMMAND_CLASS("Get", 48, "1.level", 0, get_all_motion_events, "Get Motion event count from all cameras")
    VDEV_ADD_COMMAND_CLASS("GetModelInfo", 114, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_COMMAND("MotionDetected", 2, set_camera_motion_detected, "Set camera motion detected event (args: camera name, bool)")
    VDEV_ADD_COMMAND("SetArmed", 1, set_camera_armed, "Arm/Disarm camera motion detection")


    DT_BLINK_CAMERA = vdev_id;
    blink_camera_list = stack_create();

    event_register_handler(DT_BLINK_CAMERA, TIMER_TICK_EVENT, timer_tick_handler, NULL);
}

void        device_start()
{
    LOG_INFO(DT_BLINK_CAMERA, "Starting Blink Camera device");

    service_method_t* resolver_get_instances = builtin_service_manager_get_method("Resolver", "GetListOfInstances");

    variant_t* node_id_variant = variant_create_int32(DT_INT32, DT_BLINK_CAMERA);
    variant_t* cmd_class_variant = variant_create_int32(DT_INT32, 48);

    variant_t* blink_camera_list_variant = service_eval_method(resolver_get_instances, node_id_variant, cmd_class_variant);
    variant_free(node_id_variant);
    variant_free(cmd_class_variant);

    variant_stack_t* record_list = VARIANT_GET_PTR(variant_stack_t, blink_camera_list_variant);

    stack_for_each(record_list, record_variant)
    {
        device_record_t* device_record = VARIANT_GET_PTR(device_record_t, record_variant);
        blink_camera_add(device_record);
    }

    variant_free(blink_camera_list_variant);

    LOG_INFO(DT_BLINK_CAMERA, "Blink Camera device started");
}

variant_t*  get_model_info(device_record_t* record, va_list args)
{
    char* model_info = "{\"vendor\":\"Alex\"}";
    return variant_create_string(strdup(model_info));
}

variant_t*  get_all_motion_events(device_record_t* record, va_list args)
{
    blink_camera_entry_t* entry = blink_camera_find_instance(record->instanceId);
    if(NULL != entry)
    {
        return variant_create_bool(entry->camera_motion_detected_event);
    }
    else
    {
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
        vdev_post_event(DT_BLINK_CAMERA, 48, instance, VDEV_DATA_CHANGE_EVENT, (void*)entry->camera_motion_detected_event);
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

void timer_tick_handler(event_t* pevent, void* context)
{
    service_event_data_t* timer_event_data = (service_event_data_t*)variant_get_ptr(pevent->data);

    stack_for_each(blink_camera_list, camera_entry_variant)
    {
        blink_camera_entry_t* entry = VARIANT_GET_PTR(blink_camera_entry_t, camera_entry_variant);

        if(++(entry->active_event_tick_counter) > EVENT_ACTIVE_TIMEOUT_SEC)
        {
            LOG_DEBUG(DT_BLINK_CAMERA, "Reset active events for %s", entry->name);
            update_camera_motion_detected_state(entry->instance, false);
        }
    }
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

    service_call_method("IFTTT", "Trigger", ifttt_event);
    variant_free(ifttt_event);

    return variant_create_bool(true);
}
