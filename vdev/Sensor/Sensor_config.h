#pragma once
#include <stack.h>
#include <resolver.h>

int  DT_SENSOR;

typedef enum SensorState
{
    IDLE, ALERTED
} SensorState;

typedef struct sensor_entry_t
{
    char* name;
    int instance;
    SensorState state;
} sensor_entry_t;

variant_stack_t* sensor_list;

sensor_entry_t*    sensor_add(int instance, const char* name);
bool               sensor_del(int instance);
sensor_entry_t*    sensor_find_instance(int instance);
