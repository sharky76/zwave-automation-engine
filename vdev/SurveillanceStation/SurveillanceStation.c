#include <vdev.h>
#include "resolver.h"
#include "SurveillanceStation_cli.h"
#include "SS_config.h"
#include "SS_api.h"
#include <stdio.h>
#include <logger.h>
#include <crc32.h>
#include <event.h>
#include <event_log.h>

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
hash_table_t* SS_camera_info_table;
int  DT_SURVEILLANCE_STATION;
bool SS_device_started;

static int timer_tick_counter;
static int active_event_tick_counter;
variant_t*  get_motion_events(device_record_t* record,va_list args);
variant_t*  get_all_motion_events(device_record_t* record, va_list args);
variant_t*  get_camera_list(device_record_t* record,va_list args);
variant_t*  get_camera_name_by_id(device_record_t* record,va_list args);
variant_t*  get_snapshot_path(device_record_t* record,va_list args);
variant_t*  get_event_list(device_record_t* record,va_list args);
variant_t*  get_event_snapshot(device_record_t* record,va_list args);
variant_t*  get_event_snapshot_range(device_record_t* record,va_list args);
variant_t*  get_camera_snapshot(device_record_t* record,va_list args);
variant_t*  get_model_info(device_record_t* record, va_list args);
variant_t*  get_stm_url_path(device_record_t* record, va_list args);
variant_t*  get_stm_config(device_record_t* record, va_list args);

void        device_start(); 
void        timer_tick_handler(event_pump_t* pump, int timer_id, void* context);
void        reset_active_events_handler(event_pump_t* pump, int timer_id, void* context);
void        SS_on_data_change_event(event_t* event, void* context);

void    vdev_create(vdev_t** vdev, int vdev_id)
{
    VDEV_INIT("SurveillanceStation", device_start)

    // Mimic the ZWAVE SENSOR_BINARY data holder path (1.level)
    VDEV_ADD_COMMAND_CLASS("Get", COMMAND_CLASS_MOTION_EVENTS, "1.level", 0, get_all_motion_events, "Get Motion event count from all cameras")
    VDEV_ADD_COMMAND_CLASS("GetModelInfo", COMMAND_CLASS_MODEL_INFO, NULL, 0, get_model_info, "Get model info")
    VDEV_ADD_COMMAND_CLASS("GetCameraConfig", COMMAND_CLASS_CAMERA, NULL, 0, get_stm_config, "Get Camera stream config");

    VDEV_ADD_COMMAND("GetEvents", 1, get_motion_events, "Get Motion event count (arg: id)")
    VDEV_ADD_COMMAND("GetCameraName", 1, get_camera_name_by_id, "Get Camera name (arg: id)")
    VDEV_ADD_COMMAND("GetCameraList", 0, get_camera_list, "Get Camera list")
    VDEV_ADD_COMMAND("GetSnapshotUrl", 1, get_snapshot_path, "Get camera snapshot URL (arg: id)")
    VDEV_ADD_COMMAND("GetCameraSnapshot", 1, get_camera_snapshot, "Get camera snapshot (arg: id)")
    VDEV_ADD_COMMAND("GetEventList", 1, get_event_list, "Get most recent list of event IDs (arg: id)")
    VDEV_ADD_COMMAND("GetEventSnapshot", 2, get_event_snapshot, "Get recent event snapshot (arg: camid, eventid: -1 = Last event ID)")
    VDEV_ADD_COMMAND("GetEventSnapshotRange", 3, get_event_snapshot_range, "Get list of event snapshots in range (arg: camid, startTime, endTime)")
    VDEV_ADD_COMMAND("GetStmUrlPath", 1, get_stm_url_path, "Get camera video stream URL (arg: cam_id)")

    VDEV_ADD_CONFIG_PROVIDER(SurveillanceStation_get_config);
    
    DT_SURVEILLANCE_STATION = vdev_id;
    //event_register_handler(DT_SURVEILLANCE_STATION, TIMER_TICK_EVENT, timer_tick_handler, NULL);
    event_pump_t* pump = event_dispatcher_get_pump("TIMER_PUMP");

    int query_timer_id = event_dispatcher_register_handler(pump, QUERY_RATE_SEC*1000, false, timer_tick_handler, NULL);
    //int event_timer_id = event_dispatcher_register_handler(pump, EVENT_ACTIVE_TIMEOUT_SEC*1000, false, reset_active_events_handler, NULL);
    pump->start(pump, query_timer_id);
    //pump->start(pump, event_timer_id);

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
    SS_camera_info_table = variant_hash_init();
}

void    vdev_cli_create(cli_node_t* parent_node)
{
    SurveillanceStation_cli_create(parent_node);
}

SS_event_keeper_t* get_event_keeper(int cam_id)
{
    SS_event_keeper_t* keeper = NULL;

    variant_t* cam_info_var = variant_hash_get(SS_camera_info_table, cam_id);
    SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_info_var);

    if(cam_info != NULL)
    {
        char cam_name_buf[256] = {0};
        snprintf(cam_name_buf, 255, "%d-%s", cam_info->id, cam_info->name);
        uint32_t key = crc32(0, cam_name_buf, strlen(cam_name_buf));
        variant_t* keeper_var = variant_hash_get(SS_event_keeper_table, key);
        //printf("Cam name: %s, keeper: %p\n", cam_info->name, keeper_var);
        if(NULL != keeper_var)
        {
            keeper = (SS_event_keeper_t*)variant_get_ptr(keeper_var);
        }
    }

    return keeper;
}

variant_t*  get_motion_events(device_record_t* record,va_list args)
{
    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);
    SS_event_keeper_t* keeper = get_event_keeper(cam_id);

    if(NULL != keeper)
    {
        return variant_create_int32(DT_INT32, keeper->event_count);
    }

    return variant_create_int32(DT_INT32, 0);
}

void    aggregate_motion_events(hash_node_data_t* node_data, void* arg)
{
    int* event_count = (int*)arg;
    SS_event_keeper_t* ev = (SS_event_keeper_t*)variant_get_ptr(node_data->data);
    *event_count += ev->event_count;
}

variant_t*  get_all_motion_events(device_record_t* record, va_list args)
{
    LOG_DEBUG(DT_SURVEILLANCE_STATION, "Quierying camera ID: %d", record->instanceId);
    SS_event_keeper_t* keeper = get_event_keeper(record->instanceId);

    //int total_event_count = 0;
    //variant_hash_for_each(SS_event_keeper_table, aggregate_motion_events, (void*)&total_event_count);

    //return variant_create_int32(DT_INT32, total_event_count);
    if(NULL != keeper)
    {
        LOG_DEBUG(DT_SURVEILLANCE_STATION, "Return %d motion events for %s", keeper->event_count, keeper->camera_name);
        return variant_create_bool(keeper->event_count > 0);
    }
    else
    {
        LOG_ERROR(DT_SURVEILLANCE_STATION, "Camera ID: %d not found", record->instanceId);
        return variant_create_bool(false);
    }
}


variant_t*  get_event_list(device_record_t* record,va_list args)
{
    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);
    SS_event_keeper_t* keeper = get_event_keeper(cam_id);

    if(NULL != keeper)
    {
        variant_stack_t*  event_id_list = stack_create();
        stack_for_each(keeper->events_info_stack, event_info_variant)
        {
            SS_event_info_t* event_info = VARIANT_GET_PTR(SS_event_info_t, event_info_variant);
            stack_push_back(event_id_list, variant_create_int32(DT_INT32, event_info->event_id));
        }

        //printf("Creating list of %d events\n", keeper->events_info_stack->count);
        return variant_create_list(event_id_list);
    }

    //printf("No events found\n");
    return variant_create_bool(false);
    //SS_api_get_events_info(cam_id);
}

variant_t*  get_event_snapshot(device_record_t* record, va_list args)
{
   // printf("Event snapshot start\n");

    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);

    variant_t* event_id_var = va_arg(args, variant_t*);
    int event_id = variant_get_int(event_id_var);

    //printf("Event snapshot for cam %d and event %d\n", cam_id, event_id);

    SS_event_keeper_t* keeper = get_event_keeper(cam_id);
    if(NULL != keeper)
    {
        if(-1 != event_id)
        {
            //variant_stack_t*  event_id_list = stack_create();
            stack_for_each(keeper->events_info_stack, event_info_variant)
            {
                SS_event_info_t* event_info = VARIANT_GET_PTR(SS_event_info_t, event_info_variant);
                if(event_info->event_id == event_id)
                {
                    return variant_create_string(strdup(event_info->snapshot));
                }
            }
        }
        else
        {
            // Just return last snapshot
            SS_event_info_t* event_info = VARIANT_GET_PTR(SS_event_info_t, stack_peek_back(keeper->events_info_stack));
            if(NULL != event_info)
            {
                return variant_create_string(strdup(event_info->snapshot));
            }
        }
    }

    return variant_create_bool(false);
}

variant_t*  get_event_snapshot_range(device_record_t* record, va_list args)
{
    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);

    variant_t* start_var = va_arg(args, variant_t*);
    int start_time = variant_get_int(start_var);

    variant_t* end_var = va_arg(args, variant_t*);
    int end_time = variant_get_int(end_var);

}

variant_t* get_snapshot_path(device_record_t* record, va_list args)
{
    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);

    variant_t* cam_info_var = variant_hash_get(SS_camera_info_table, cam_id);
    SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_info_var);

    if(cam_info != NULL)
    {
        char buf[512] = {0};
        //SS_api_get_sid();
        snprintf(buf, 511, "%s%s&_sid=%s", SS_base_url, cam_info->snapshot_path, SS_auth_sid);
        return variant_create_string(strdup(buf));
    }
    else
    {
        return variant_create_string(strdup(""));
    }
}

variant_t*  get_camera_name_by_id(device_record_t* record, va_list args)
{
    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);

    variant_t* cam_info_var = variant_hash_get(SS_camera_info_table, cam_id);
    SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_info_var);

    if(cam_info != NULL)
    {
        return variant_create_string(strdup(cam_info->name));
    }
    else
    {
        return variant_create_string(strdup(""));
    }
}

/*typedef struct camera_info_context_t
{
    char** list;
    int    index;
} camera_info_context_t;*/

void process_camera_info_table(hash_node_data_t* node_data, void* arg)
{
    //camera_info_context_t* ctx = (camera_info_context_t*)arg;
    variant_stack_t* cam_list = (variant_stack_t*)arg;
    char cam_name_buf[256] = {0};
    SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(node_data->data);

    snprintf(cam_name_buf, 255, "%d: %s", cam_info->id, cam_info->name);

    //ctx->list[ctx->index++] = strdup(cam_name_buf);
    stack_push_back(cam_list, variant_create_string(strdup(cam_name_buf)));

    LOG_DEBUG(DT_SURVEILLANCE_STATION, "%s", cam_name_buf);
}

variant_t*  get_camera_list(device_record_t* record, va_list args)
{
    if(SS_camera_info_table->count == 0)
    {
        SS_api_query();
        SS_api_get_sid();
    }

    SS_api_get_camera_list();
    //SS_api_logout();

    //char** cam_list = malloc(sizeof(char*) * (SS_camera_info_table->count + 1));

    variant_stack_t* cam_list = stack_create();

    /*camera_info_context_t data_context = {
        .list = cam_list,
        .index = 0
    };*/

    variant_hash_for_each(SS_camera_info_table, process_camera_info_table, cam_list);

    //cam_list[data_context.index] = NULL;
    return variant_create_list(cam_list);
}

void update_resolver_entries(hash_node_data_t* node_data, void* arg)
{
    SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(node_data->data);
    resolver_add_entry(VDEV, cam_info->name, DT_SURVEILLANCE_STATION, cam_info->id, COMMAND_CLASS_MOTION_EVENTS);
    resolver_add_entry_default(VDEV, DT_SURVEILLANCE_STATION, cam_info->id, COMMAND_CLASS_CAMERA);

    cam_info->timer_id = event_dispatcher_register_handler(event_dispatcher_get_pump("TIMER_PUMP"), 
                                EVENT_ACTIVE_TIMEOUT_SEC*1000, true, reset_active_events_handler, (void*)cam_info);
}

void    device_start()
{
    LOG_INFO(DT_SURVEILLANCE_STATION, "Starting Surveillance Station device");
    SS_api_query();
    SS_api_get_sid();
    SS_api_get_info();
    SS_api_get_camera_list();
    //SS_api_logout();

    // Update Resolver table with all found cameras...
    variant_hash_for_each(SS_camera_info_table, update_resolver_entries, NULL);

    LOG_INFO(DT_SURVEILLANCE_STATION, "Surveillance Station device started");
    SS_device_started = true;
}

void SS_on_data_change_event(event_t* event, void* context)
{
    vdev_event_data_t* event_data = (vdev_event_data_t*)variant_get_ptr(event->data);
    SS_event_keeper_t* ev = (SS_event_keeper_t*)event_data->data;
    
    char json_buf[256] = {0};
    
    snprintf(json_buf, 255, "{\"camera_id\":\"%d\",\"camera_name\":\"%s\",\"event_count\":\"%d\",\"data_holder\":1,\"level\":%s}",
             ev->camera_id, 
             ev->camera_name,
             ev->event_count,
             (ev->event_active)? "true" : "false");

    event_log_entry_t* new_entry = calloc(1, sizeof(event_log_entry_t));
    new_entry->node_id = event_data->vdev_id;
    new_entry->instance_id = event_data->instance_id;
    new_entry->command_id = event_data->event_id;
    new_entry->device_type = VDEV;
    new_entry->event_data = strdup(json_buf);
    event_log_add_event(new_entry);
 }

void SS_emit_data_change_event(SS_event_keeper_t* ev)
{
    char json_buf[256] = {0};
    
    snprintf(json_buf, 255, "{\"camera_id\":\"%d\",\"camera_name\":\"%s\",\"event_count\":\"%d\",\"data_holder\":1,\"level\":%s}",
             ev->camera_id, 
             ev->camera_name,
             ev->event_count,
             (ev->event_active)? "true" : "false");

    event_log_entry_t* new_entry = calloc(1, sizeof(event_log_entry_t));
    new_entry->node_id = DT_SURVEILLANCE_STATION;
    new_entry->instance_id = ev->camera_id;
    new_entry->command_id = COMMAND_CLASS_MOTION_EVENTS;
    new_entry->device_type = VDEV;
    new_entry->event_data = strdup(json_buf);
    event_log_add_event(new_entry);
 }

void    process_motion_event_table(hash_node_data_t* node_data, void* arg)
{
    SS_event_keeper_t* ev = (SS_event_keeper_t*)variant_get_ptr(node_data->data);
    LOG_DEBUG(DT_SURVEILLANCE_STATION, "Key: %u value: %s %d, old: %d", node_data->key, ev->camera_name, ev->event_count, ev->old_event_count);
    if(ev->event_count > ev->old_event_count)
    {
        ev->event_active = true;
        ev->active_event_tick_counter = 0;
        SS_api_get_events_info(ev);
        LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Motion detected event on camera: %s with ID %d", ev->camera_name, ev->camera_id);
        vdev_post_event(DT_SURVEILLANCE_STATION, COMMAND_CLASS_MOTION_EVENTS, ev->camera_id, VDEV_DATA_CHANGE_EVENT, ev);
        SS_emit_data_change_event(ev);

        event_pump_t* timer_pump = event_dispatcher_get_pump("TIMER_PUMP");
        variant_t* cam_info_var = variant_hash_get(SS_camera_info_table, ev->camera_id);
        SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_info_var);
        timer_pump->start(timer_pump, cam_info->timer_id);
    }

    ev->old_event_count = ev->event_count;
}

void    reset_active_events(hash_node_data_t* node_data, void* arg)
{
    SS_camera_info_t* cam_info = (SS_camera_info_t*)arg;
    SS_event_keeper_t* ev = (SS_event_keeper_t*)variant_get_ptr(node_data->data);
    
    if(cam_info->id != ev->camera_id) return;

    //LOG_DEBUG(DT_SURVEILLANCE_STATION, "Key: %u value: %s %d, old: %d", node_data->key, ev->camera_name, ev->event_count, ev->old_event_count);
    if(ev->event_active)
    {
        LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Reset active events for camera %s (was %d)", ev->camera_name, ev->event_count);
        ev->event_active = false;
        ev->event_count = ev->old_event_count = 0;
        ev->active_event_tick_counter = 0;
        vdev_post_event(DT_SURVEILLANCE_STATION, COMMAND_CLASS_MOTION_EVENTS, ev->camera_id, VDEV_DATA_CHANGE_EVENT, ev);
        SS_emit_data_change_event(ev);
    }
}

void        timer_tick_handler(event_pump_t* pump, int timer_id, void* context)
{

    LOG_DEBUG(DT_SURVEILLANCE_STATION, "Timer event received");

    // Check for outstanding motion events...
    //SS_api_get_sid();
    SS_api_get_motion_events();
    //SS_api_logout();
    variant_hash_for_each(SS_event_keeper_table, process_motion_event_table, NULL);

    
}

void        reset_active_events_handler(event_pump_t* pump, int timer_id, void* context)
{
    variant_hash_for_each(SS_event_keeper_table, reset_active_events, context);
}

variant_t*  get_camera_snapshot(device_record_t* record, va_list args)
{
    char* snapshot;
    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);

    SS_api_get_camera_snapshot(cam_id, &snapshot);
    return variant_create_string(snapshot);
}

variant_t*  get_model_info(device_record_t* record, va_list args)
{
    char* model_info = "{\"vendor\":\"Synology\"}";
    return variant_create_string(strdup(model_info));
}

variant_t*  get_stm_url_path(device_record_t* record, va_list args)
{
    variant_t* camera_id_var = va_arg(args, variant_t*);
    int cam_id = variant_get_int(camera_id_var);

    variant_t* cam_info_var = variant_hash_get(SS_camera_info_table, cam_id);
    SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_info_var);

    if(cam_info != NULL && NULL != cam_info->stm_url_path)
    {
        return variant_create_string(strdup(cam_info->stm_url_path));
    }

    return variant_create_bool(false);
}

variant_t*  get_stm_config(device_record_t* record, va_list args)
{
    variant_t* cam_info_var = variant_hash_get(SS_camera_info_table, record->instanceId);
    
    if(NULL != cam_info_var)
    {
        SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_info_var);

        char* stm_info = "{ \"cameras\": [{"\
        "\"name\": \"%s\","\
        "\"videoConfig\": {"\
        "\"source\": \"-re -i rtsp://%s \","\
        "\"maxStreams\": %d,"\
        "\"maxWidth\": %d,"\
        "\"maxHeight\": %d,"\
        "\"maxFPS\": %d,"\
        "}}]}";

        char buf[1024] = {0};
        snprintf(buf, 1023, stm_info, cam_info->name, 
                                      cam_info->stm_url_path,
                                      cam_info->num_streams, 
                                      cam_info->max_width, 
                                      cam_info->max_height, 
                                      cam_info->max_fps);
                                      
        return variant_create_string(strdup(buf));
    }

    return variant_create_bool(false);
}