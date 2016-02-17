#include "DateTime.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "datetime_cli.h"

variant_t*  datetime_get_time_string(service_method_t* method, va_list args);
variant_t*  datetime_get_timestamp(service_method_t* method, va_list args);

void    service_create(service_t** service, int service_id)
{
    SERVICE_INIT(service, DateTime, "Provides date/time display")
    SERVICE_ADD_METHOD(service, GetTimeString, datetime_get_time_string, 0, "Returns date formatted as string")
    SERVICE_ADD_METHOD(service, GetTimeStamp, datetime_get_timestamp, 0, "Returns date as UNIX timestamp")

    (*service)->get_config_callback = datetime_cli_get_config;

    datetime_time_format = strdup("%H:%M");
}

void    service_cli_create(cli_node_t* parent_node)
{
    datetime_cli_init(parent_node);
}

variant_t*  datetime_get_time_string(service_method_t* method, va_list args)
{
    time_t t = time(NULL);
    struct tm* p_tm = localtime(&t);

    char* timebuf = (char*)calloc(10, sizeof(char));

    strftime(timebuf, 10, datetime_time_format, p_tm);

    return variant_create(DT_STRING, timebuf);
}

variant_t*  datetime_get_timestamp(service_method_t* method, va_list args)
{
    time_t t = time(NULL);
    return variant_create_int32(DT_INT32, t);
}

