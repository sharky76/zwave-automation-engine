#include <vdev.h>
#include "SurveillanceStation_cli.h"
#include "SS_config.h"
#include "SS_api.h"
#include <stdio.h>
#include <logger.h>
#include <crc32.h>

char* SS_user;
char* SS_pass;
char* SS_base_url;
char* SS_auth_sid;
char* SS_auth_path;
char* SS_event_path;
char* SS_camera_path;
char* SS_info_path;
int   SS_camera_count;
hash_table_t*   SS_event_keeper_table;
int  DT_SURVEILLANCE_STATION;
bool SS_device_started;

static int timer_tick_counter;
variant_t*  get_motion_events(va_list args);
void        device_start(); 
void        timer_tick_handler(const char* event_name, event_t* pevent);

void    vdev_create(vdev_t** vdev, int vdev_id)
{
    VDEV_INIT("SurveillanceStation", device_start)
    VDEV_ADD_COMMAND(1, "GetEvents", 1, get_motion_events, "Get Event status")
    VDEV_ADD_CONFIG_PROVIDER(SurveillanceStation_get_config);
    VDEV_SUBSCRIBE_TO_EVENT_SOURCE("Timer", timer_tick_handler);

    DT_SURVEILLANCE_STATION = vdev_id;

    SS_user = NULL;
    SS_pass = NULL;
    SS_base_url = NULL;
    SS_auth_sid = NULL;
    SS_auth_path = NULL;
    SS_event_path = NULL;
    SS_camera_path = NULL;
    SS_info_path = NULL;
    SS_device_started = false;

    SS_event_keeper_table = variant_hash_init();
}

void    vdev_cli_create(cli_node_t* parent_node)
{
    SurveillanceStation_cli_create(parent_node);
}

variant_t*  get_motion_events(va_list args)
{
    variant_t* camera_name_var = va_arg(args, variant_t*);
    const char* camera_name = variant_get_string(camera_name_var);

    uint32_t key = crc32(0, camera_name, strlen(camera_name));
    variant_t* keeper_var = variant_hash_get(SS_event_keeper_table, key);

    if(NULL != keeper_var)
    {
        SS_event_keeper_t* keeper = (SS_event_keeper_t*)variant_get_ptr(keeper_var);
        return variant_create_int32(DT_INT32, keeper->event_count);
    }

    return variant_create_int32(DT_INT32, 0);
}

void    device_start()
{
    LOG_INFO(DT_SURVEILLANCE_STATION, "Starting Surveillance Station device");
    SS_api_query();
    SS_api_get_sid();
    SS_api_get_info();
    LOG_INFO(DT_SURVEILLANCE_STATION, "Surveillance Station device started");
    SS_device_started = true;
}


void    process_motion_event_table(hash_node_data_t* node_data, void* arg)
{
    SS_event_keeper_t* ev = (SS_event_keeper_t*)variant_get_ptr(node_data->data);
    LOG_DEBUG(DT_SURVEILLANCE_STATION, "Key: %u value: %s %d, old: %d", node_data->key, ev->camera_name, ev->event_count, ev->old_event_count);
    if(ev->event_count > ev->old_event_count)
    {
        LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Motion detected event on camera: %s", ev->camera_name);
        ev->old_event_count = ev->event_count;
        vdev_post_event(DT_SURVEILLANCE_STATION, 1);
    }
}

void        timer_tick_handler(const char* event_name, event_t* pevent)
{
    service_event_data_t* timer_event_data = (service_event_data_t*)variant_get_ptr(pevent->data);

    if(++timer_tick_counter > QUERY_RATE_SEC && strcmp(timer_event_data->data, "tick") == 0)
    {
        timer_tick_counter = 0;

        // Check for outstanding motion events...
        SS_api_get_motion_events();
        variant_hash_for_each(SS_event_keeper_table, process_motion_event_table, NULL);
    }
}
