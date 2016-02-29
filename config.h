#pragma once
#include <stdbool.h>

/*
This is the configuration source for main app
*/

typedef struct config_t
{
    char*   device;
    char*   api_prefix;
    char*   services_prefix;
    char*   config_location;
    int     client_port;
} config_t;

extern config_t    global_config;

bool    config_load(const char* file);

