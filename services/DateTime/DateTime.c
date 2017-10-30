#include "DateTime.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "datetime_cli.h"
#include "logger.h"

variant_t*  datetime_get_time_string(service_method_t* method, va_list args);
variant_t*  datetime_get_timestamp(service_method_t* method, va_list args);
variant_t*  datetime_time_from_timesamp(service_method_t* method, va_list args);
variant_t*  datetime_time_less_than(service_method_t* method, va_list args);
variant_t*  datetime_time_greater_than(service_method_t* method, va_list args);
variant_t*  datetime_get_date_string(service_method_t* method, va_list args);

int DT_DATETIME;

void    service_create(service_t** service, int service_id)
{
    SERVICE_INIT(DateTime, "Provides date/time display")
    SERVICE_ADD_METHOD(GetTimeString, datetime_get_time_string, 0, "Returns time formatted as string (hours from 0-24, minutes from 00-59)")
    SERVICE_ADD_METHOD(FromTimeStamp, datetime_time_from_timesamp, 1, "Convert timestamp to time formatted as string (hours from 0-24, minutes from 00-59)")

    SERVICE_ADD_METHOD(GetTimeStamp,  datetime_get_timestamp, 0, "Returns time as UNIX timestamp")
    SERVICE_ADD_METHOD(TimeLessThan,  datetime_time_less_than, 1, "Compare current time with string")
    SERVICE_ADD_METHOD(TimeGreaterThan,  datetime_time_greater_than, 1, "Compare current time with string")
    SERVICE_ADD_METHOD(GetDateString, datetime_get_date_string, 0, "Returns date formatted as string")


    (*service)->get_config_callback = datetime_cli_get_config;
    DT_DATETIME = service_id;
    datetime_time_format = strdup("%H:%M");
    datetime_date_format = strdup("%m/%d/%Y");
}

void    service_cli_create(cli_node_t* parent_node)
{
    datetime_cli_init(parent_node);
}

variant_t*  datetime_get_time_string(service_method_t* method, va_list args)
{
    time_t t = time(NULL);
    struct tm* p_tm = localtime(&t);

    char* timebuf = (char*)calloc(200, sizeof(char));

    strftime(timebuf, 200, datetime_time_format, p_tm);

    return variant_create_string(timebuf);
}

variant_t*  datetime_time_from_timesamp(service_method_t* method, va_list args)
{
    variant_t* timestamp_variant = va_arg(args, variant_t*);
    time_t t = (time_t)variant_get_int(timestamp_variant);
    struct tm* p_tm = localtime(&t);

    char* timebuf = (char*)calloc(200, sizeof(char));

    strftime(timebuf, 200, datetime_time_format, p_tm);

    return variant_create_string(timebuf);
}

variant_t*  datetime_get_timestamp(service_method_t* method, va_list args)
{
    time_t t = time(NULL);
    return variant_create_int32(DT_INT32, t);
}

variant_t*  datetime_time_less_than(service_method_t* method, va_list args)
{
    variant_t* time_variant = va_arg(args, variant_t*);
    const char* time_string = variant_get_string(time_variant);
    
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(time_string, datetime_time_format, &tm);
    int compare_time = tm.tm_sec + tm.tm_min*60 + tm.tm_hour*3600;

    time_t t = time(NULL);
    struct tm* p_tm = localtime(&t);
    char* timebuf = (char*)calloc(200, sizeof(char));
    strftime(timebuf, 200, datetime_time_format, p_tm);
    memset(&tm, 0, sizeof(struct tm));
    strptime(timebuf, datetime_time_format, &tm);
    int current_time = tm.tm_sec + tm.tm_min*60 + tm.tm_hour*3600;
    free(timebuf);

    //printf("datetime_time_less_than compare %d with %d\n", compare_time, current_time);
    return variant_create_bool(current_time < compare_time);
}

variant_t*  datetime_time_greater_than(service_method_t* method, va_list args)
{
    variant_t* time_variant = va_arg(args, variant_t*);
    const char* time_string = variant_get_string(time_variant);
    
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(time_string, datetime_time_format, &tm);
    int compare_time = tm.tm_sec + tm.tm_min*60 + tm.tm_hour*3600;

    time_t t = time(NULL);
    struct tm* p_tm = localtime(&t);
    char* timebuf = (char*)calloc(200, sizeof(char));
    strftime(timebuf, 200, datetime_time_format, p_tm);
    memset(&tm, 0, sizeof(struct tm));
    strptime(timebuf, datetime_time_format, &tm);
    int current_time = tm.tm_sec + tm.tm_min*60 + tm.tm_hour*3600;
    free(timebuf);

    //printf("datetime_time_greater_than compare %d with %d\n", compare_time, current_time);

    return variant_create_bool(current_time > compare_time);

}

variant_t*  datetime_get_date_string(service_method_t* method, va_list args)
{
    time_t t = time(NULL);
    struct tm* p_tm = localtime(&t);

    char* datebuf = (char*)calloc(200, sizeof(char));

    strftime(datebuf, 200, datetime_date_format, p_tm);

    LOG_DEBUG(DT_DATETIME, "Date %s with format %s", datebuf, datetime_date_format);

    return variant_create_string(datebuf);
}

