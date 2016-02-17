#pragma once

#include <ZWayLib.h>
#include <ZData.h>

/*
    This file holds callback(s) to handle data changes. Each data change is considered to be
    scene event and an appropriate scene definition lookup is performed and, if found,
    the scene is executed
*/

typedef struct device_event_data_t
{
    ZWBYTE  node_id;
    ZWBYTE  instance_id;
    ZWBYTE  command_id;
    const char*   device_name;
    double  last_update_time;
} device_event_data_t;

void data_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg);


