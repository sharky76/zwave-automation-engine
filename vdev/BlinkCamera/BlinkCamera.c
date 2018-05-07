#include <vdev.h>
#include <stdio.h>
#include <logger.h>
#include <crc32.h>
#include <event.h>
#include <event_log.h>
#include "service.h"
#include "service_args.h"

#define EVENT_ACTIVE_TIMEOUT_SEC 180

int  DT_BLINK_CAMERA;
static int  active_event_tick_counter;
static bool  camera_motion_detected_event;

void        device_start(); 
variant_t*  get_model_info(va_list args);
variant_t*  get_all_motion_events(va_list args);
variant_t*  set_camera_motion_detected(va_list args);
variant_t*  set_camera_armed(va_list args);

void        timer_tick_handler(event_t* pevent, void* context);

void    vdev_create(vdev_t** vdev, int vdev_id)
{
    VDEV_INIT("BlinkCamera", device_start)

    VDEV_ADD_COMMAND_CLASS("Get", 48, "1.level", 0, get_all_motion_events, "Get Motion event count from all cameras")
    VDEV_ADD_COMMAND_CLASS("GetModelInfo", 114, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_COMMAND("MotionDetected", 1, set_camera_motion_detected, "Set camera motion detected event")
    VDEV_ADD_COMMAND("SetArmed", 1, set_camera_armed, "Arm/Disarm camera motion detection")


    DT_BLINK_CAMERA = vdev_id;
    camera_motion_detected_event = false;
    event_register_handler(DT_BLINK_CAMERA, TIMER_TICK_EVENT, timer_tick_handler, NULL);
}

void        device_start()
{
    LOG_INFO(DT_BLINK_CAMERA, "Starting Blink Camera device");

    LOG_INFO(DT_BLINK_CAMERA, "Blink Camera device started");
}

variant_t*  get_model_info(va_list args)
{
    char* model_info = "{\"vendor\":\"Alex\"}";
    return variant_create_string(strdup(model_info));
}

variant_t*  get_all_motion_events(va_list args)
{
    return variant_create_bool(camera_motion_detected_event);
}

void update_camera_motion_detected_state(bool state)
{
    camera_motion_detected_event = state;

    LOG_INFO(DT_BLINK_CAMERA, "Blink Camera motion detection set to %d\n", camera_motion_detected_event);
    char json_buf[256] = {0};

    // Generate REST API event
    snprintf(json_buf, 255, "{\"data_holder\":\"1\",\"level\":%s}",
             (camera_motion_detected_event)? "true" : "false");

    event_log_entry_t* new_entry = calloc(1, sizeof(event_log_entry_t));
    new_entry->node_id = DT_BLINK_CAMERA;
    new_entry->instance_id = 0;
    new_entry->command_id = 48;
    new_entry->device_type = VDEV;
    new_entry->event_data = strdup(json_buf);
    event_log_add_event(new_entry);

    // Notify system about security state change
    vdev_post_event(DT_BLINK_CAMERA, 48, 0, VDEV_DATA_CHANGE_EVENT, (void*)camera_motion_detected_event);
}

variant_t*  set_camera_motion_detected(va_list args)
{
    variant_t* new_state_variant = va_arg(args, variant_t*);
    update_camera_motion_detected_state(variant_get_bool(new_state_variant));

    return variant_create_bool(true);
}

void timer_tick_handler(event_t* pevent, void* context)
{
    service_event_data_t* timer_event_data = (service_event_data_t*)variant_get_ptr(pevent->data);

    if(++active_event_tick_counter > EVENT_ACTIVE_TIMEOUT_SEC)
    {
        LOG_DEBUG(DT_BLINK_CAMERA, "Reset active events");
        active_event_tick_counter = 0;
        update_camera_motion_detected_state(false);
    }
}

variant_t*  set_camera_armed(va_list args)
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
