#include "data_callbacks.h"
#include <event.h>
#include <logger.h>
#include <ZWayLib.h>
#include "variant_types.h"
#include "resolver.h"
#include <ZLogging.h>
#include <stdio.h>
#include "device_callbacks.h"
#include <time.h>
#include "hash.h"
#include "crc32.h"
#include "command_class.h"
#include "event_log.h"
#include "zway_json.h"

extern ZWay zway;
extern hash_table_t*   data_holder_event_table;

DECLARE_LOGGER(DataCallback)

void dump_data(const ZWay zway, ZDataHolder data)
{
    char *path = zdata_get_path(data);
    ZWDataType type;
    zdata_get_type(data, &type);

    ZWBOOL bool_val;
    int int_val;
    float float_val;
    ZWCSTR str_val;
    const ZWBYTE *binary;
    const int *int_arr;
    const float *float_arr;
    const ZWCSTR *str_arr;
    size_t len, i;

    switch (type) 
    {
        case Empty:
            zway_log(zway, Debug, ZSTR("DATA %s = Empty"), path);
            break;
        case Boolean:
            zdata_get_boolean(data, &bool_val);
            if (bool_val)
                zway_log(zway, Debug, ZSTR("DATA %s = True"), path);
            else
                zway_log(zway, Debug, ZSTR("DATA %s = False"), path);
            break;
        case Integer:
            zdata_get_integer(data, &int_val);
            zway_log(zway, Debug, ZSTR("DATA %s = %d (0x%08x)"), path, int_val, int_val);
            break;
        case Float:
            zdata_get_float(data, &float_val);
            zway_log(zway, Debug, ZSTR("DATA %s = %f"), path, float_val);
            break;
        case String:
            zdata_get_string(data, &str_val);
            zway_log(zway, Debug, ZSTR("DATA %s = \"%s\""), path, str_val);
            break;
        case Binary:
            zdata_get_binary(data, &binary, &len);
            zway_log(zway, Debug, ZSTR("DATA %s = byte[%d]"), path, len);
            zway_dump(zway, Debug, ZSTR("  "), len, binary);
            break;
        case ArrayOfInteger:
            zdata_get_integer_array(data, &int_arr, &len);
            zway_log(zway, Debug, ZSTR("DATA %s = int[%d]"), path, len);
            for (i = 0; i < len; i++)
                zway_log(zway, Debug, ZSTR("  [%02d] %d"), i, int_arr[i]);
            break;
        case ArrayOfFloat:
            zdata_get_float_array(data, &float_arr, &len);
            zway_log(zway, Debug, ZSTR("DATA %s = float[%d]"), path, len);
            for (i = 0; i < len; i++)
                zway_log(zway, Debug, ZSTR("  [%02d] %f"), i, float_arr[i]);
            break;
        case ArrayOfString:
            zdata_get_string_array(data, &str_arr, &len);
            zway_log(zway, Debug, ZSTR("DATA %s = string[%d]"), path, len);
            for (i = 0; i < len; i++)
                zway_log(zway, Debug, ZSTR("  [%02d] \"%s\""), i, str_arr[i]);
            break;
    }
    free(path);
    
    ZDataIterator child = zdata_first_child(data);
    while (child != NULL)
    {
        dump_data(zway,child->data);
        //path = zdata_get_path(child->data);
        //zway_log(zway, Debug, ZSTR("CHILD %s"), path);
        //free(path);
        child = zdata_next_child(child);
    }
}

/** 
 * 
 * 
 * @param data
 * @param event_data
 */
void install_callback(ZDataHolder data, sensor_event_data_t* event_data)
{
    ZDataIterator child = zdata_first_child(data);
    while (child != NULL)
    {
        char* path = zdata_get_path(child->data);
        LOG_DEBUG(DataCallback, "Adding callback for path: %s", path);
        free(path);
        zdata_add_callback_ex(child->data, &value_change_event_callback, TRUE, event_data);
        child = zdata_next_child(child);
    }

    event_data->callback_added = true;

}

void data_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg)
{
    sensor_event_data_t* event_data = (sensor_event_data_t*)arg;

    /*if(NULL == event_data->device_name)
    {
        event_data->device_name = resolver_name_from_id(event_data->node_id, event_data->instance_id, event_data->command_id);
    }*/

    if(changeType == ChildCreated)
    {
        install_callback(data, event_data);
    }
    else if(changeType == (Deleted & 0x0f) || changeType ==  (Invalidated & 0x0f))
    {
        //zdata_remove_callback_ex(child->data, &value_change_event_callback)
        LOG_DEBUG(DataCallback, "Command removed node-id %d, instance-id %d, command-id %d", event_data->node_id, event_data->instance_id, event_data->command_id);
    }
    else if(changeType == (ChildEvent|Updated) && !event_data->callback_added)
    {
        install_callback(data, event_data);
        //LOG_DEBUG(DataCallback, "Data change event for changeType: ChildEvent|Updated and node-id %d, instance-id %d, command-id %d", event_data->node_id, event_data->instance_id, event_data->command_id);
    }
    else if(changeType == (ChildEvent|PhantomUpdate|Updated))
    {
        //LOG_DEBUG(DataCallback, "Data change event for changeType: ChildEvent|PhantomUpdate|Updated and node-id %d, instance-id %d, command-id %d", event_data->node_id, event_data->instance_id, event_data->command_id);
    }
    else
    {
        //LOG_DEBUG(DataCallback, "Data change event for changeType: %d and node-id %d, instance-id %d, command-id %d", changeType, event_data->node_id, event_data->instance_id, event_data->command_id);
    }
}

void value_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg)
{
    sensor_event_data_t* event_data = (sensor_event_data_t*)arg;
    //event_data->device_name = resolver_name_from_id(event_data->node_id, event_data->instance_id, event_data->command_id);

    if(changeType == Updated)
    {
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC, &tp);
       
        unsigned long time_msec = tp.tv_sec * 1000 + tp.tv_nsec / 1000000L;

        LOG_DEBUG(DataCallback, "Value changed for device %s time: %lu, old time: %lu, dh name: %s", event_data->device_name, time_msec, event_data->last_update_time, zdata_get_name(data));

        //if(time_msec >  event_data->last_update_time + 100)
        {
            LOG_ADVANCED(DataCallback, "Data changed for device (%s) node-id: %d, instance-id: %d, command-id: %d", event_data->device_name, event_data->node_id, event_data->instance_id, event_data->command_id);
            event_data->last_update_time = time_msec;
            
            event_pump_send_event(event_data->event_pump, SensorDataChangeEvent, (void*)event_data, NULL);

            event_log_entry_t* new_entry = calloc(1, sizeof(event_log_entry_t));
            new_entry->node_id = event_data->node_id;
            new_entry->instance_id = event_data->instance_id;
            new_entry->command_id = event_data->command_id;
            new_entry->device_type = ZWAVE;

            json_object* json_resp = json_object_new_object();
            zway_json_data_holder_to_json(json_resp, data);
            json_object* dh_name_obj = json_object_new_string(zdata_get_name(data));
            json_object_object_add(json_resp, "data_holder", dh_name_obj);
            new_entry->event_data = strdup(json_object_to_json_string_ext(json_resp, JSON_C_TO_STRING_PLAIN));
            json_object_put(json_resp);
            event_log_add_event(new_entry);
            //free(json_resp);
        }
    }
}

