#include "data_callbacks.h"
#include <event.h>
#include <logger.h>
#include <ZWayLib.h>
#include "variant_types.h"
#include "resolver.h"
#include "data_cache.h"
#include <ZLogging.h>
#include <stdio.h>
#include "device_callbacks.h"
#include "data_cache.h"
#include <time.h>

extern ZWay zway;

void value_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg);

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

void data_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg)
{
    device_event_data_t* event_data = (device_event_data_t*)arg;

    /*if(NULL == event_data->device_name)
    {
        event_data->device_name = resolver_name_from_id(event_data->node_id, event_data->instance_id, event_data->command_id);
    }*/

    if(changeType == ChildCreated)
    {
        ZDataIterator child = zdata_first_child(data);
        while (child != NULL)
        {
            zdata_add_callback_ex(child->data, &value_change_event_callback, TRUE, event_data);
            child = zdata_next_child(child);
        }
    }
    else if(changeType == (Deleted & 0x0f) || changeType ==  (Invalidated & 0x0f))
    {
        LOG_DEBUG("Command removed node-id %d, instance-id %d, command-id %d", event_data->node_id, event_data->instance_id, event_data->command_id);
    }
}

void value_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg)
{
    device_event_data_t* event_data = (device_event_data_t*)arg;

    //if(NULL == event_data->device_name)
    {
        event_data->device_name = resolver_name_from_id(event_data->node_id, event_data->instance_id, event_data->command_id);
    }

    if(changeType == Updated)
    {
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC, &tp);
       
        double time_msec = tp.tv_sec * 1000 + (double)tp.tv_nsec / 1000000L;

        if(time_msec >  event_data->last_update_time + 100)
        {
            event_data->last_update_time = time_msec;
            event_t* event = event_create(DT_SENSOR, variant_create_ptr(DT_SENSOR_EVENT_DATA, event_data, variant_delete_none));
            event_post(event);
        }
    }
}

