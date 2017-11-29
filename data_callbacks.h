#pragma once

#include <ZWayLib.h>
#include <ZData.h>
#include <stdbool.h>

#define SENSOR_DATA_CHANGE_EVENT   "SensorDataChangeEvent"

typedef struct sensor_event_data_t
{
    uint8_t  node_id;
    uint8_t  instance_id;
    uint8_t  command_id;
    const char*   device_name;
    unsigned long  last_update_time;
    ZDataHolder    data_holder;
    bool    callback_added;
} sensor_event_data_t;

/*
    This file holds callback(s) to handle data changes. Each data change is considered to be
    scene event and an appropriate scene definition lookup is performed and, if found,
    the scene is executed
*/
void data_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg);
void value_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg);
