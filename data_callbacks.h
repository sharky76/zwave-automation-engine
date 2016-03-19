#pragma once

#include <ZWayLib.h>
#include <ZData.h>

/*
    This file holds callback(s) to handle data changes. Each data change is considered to be
    scene event and an appropriate scene definition lookup is performed and, if found,
    the scene is executed
*/

void data_change_event_callback(ZDataRootObject rootObject, ZWDataChangeType changeType, ZDataHolder data, void * arg);


