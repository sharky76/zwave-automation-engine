#include "weather_methods.h"
#include "weather_cache.h"
#include <stdio.h>
#include <time.h>

variant_t*  weather_get_temp(service_method_t* method, va_list args)
{
    weather_cache_refresh();
    return variant_create_float(weather_cache.temp);
}

variant_t*  weather_get_windspeed(service_method_t* method, va_list args)
{
    weather_cache_refresh();
    return variant_create_float(weather_cache.windspeed);
}

variant_t*  weather_get_precipitation(service_method_t* method, va_list args)
{
    weather_cache_refresh();

    if(NULL == weather_cache.precipitation)
    {
        return variant_create_string(strdup("no data"));
    }

    return variant_create_string(strdup(weather_cache.precipitation));
}

variant_t*  weather_refresh_cache(service_method_t* method, va_list args)
{
    weather_cache_refresh();
    return variant_create_bool(true);
}

variant_t*  weather_get_humidity(service_method_t* method, va_list args)
{
    weather_cache_refresh();
    return variant_create_int32(DT_INT32, weather_cache.humidity);
}

variant_t*  weather_get_sunrise(service_method_t* method, va_list args)
{
    weather_cache_refresh();
    return variant_create_int32(DT_INT32, weather_cache.sunrise);
}

variant_t*  weather_get_sunset(service_method_t* method, va_list args)
{
    weather_cache_refresh();
    return variant_create_int32(DT_INT32, weather_cache.sunset);
}

variant_t*  weather_get_forecast(service_method_t* method, va_list args)
{
    weather_cache_refresh();
    variant_t* range_variant = va_arg(args, variant_t*);
    char* range;
    variant_to_string(range_variant, &range);
    
    char* formatted_output = NULL;
    if(NULL != range)
    {
        if(0 == strcmp(range, "Hourly") && weather_cache.forecast_hourly->count > 0)
        {
            stack_iterator_t* it = stack_iterator_begin(weather_cache.forecast_hourly);
            int nHours = 8;
            while(nHours-- > 0 && !stack_iterator_is_end(it))
            {
                variant_t* forecast_entry_variant = stack_iterator_data(it);
                weather_forecast_cache_t* forecast_entry = VARIANT_GET_PTR(weather_forecast_cache_t, forecast_entry_variant);
                //variant_t* vv = variant_create_float(forecast_entry->weather_entry.temp);
                char time_string[256] = {0};
                struct tm* time_tm = localtime(&forecast_entry->timestamp);
                size_t time_size = strftime(time_string, 255, "%l%P", time_tm);

                int old_len = 0;
                if(NULL != formatted_output)
                {
                    old_len = strlen(formatted_output);
                }

                formatted_output = realloc(formatted_output, old_len + 255);
                snprintf(formatted_output+old_len, 254, "%s %.1fF %s\r\n", time_string, forecast_entry->weather_entry.temp, forecast_entry->weather_entry.precipitation);

                it = stack_iterator_next(it);
            }

            stack_iterator_free(it);
            return variant_create_string(formatted_output);
        }
        else if(0 == strcmp(range, "Daily") && weather_cache.forecast_hourly->count > 0)
        {
            stack_for_each(weather_cache.forecast_hourly, forecast_entry_variant)
            {
                weather_forecast_cache_t* forecast_entry = VARIANT_GET_PTR(weather_forecast_cache_t, forecast_entry_variant);
                //variant_t* vv = variant_create_float(forecast_entry->weather_entry.temp);
                char time_string[256] = {0};
                struct tm* time_tm = localtime(&forecast_entry->timestamp);
                size_t time_size = strftime(time_string, 255, "%a %e %l%P", time_tm);

                int old_len = 0;
                if(NULL != formatted_output)
                {
                    old_len = strlen(formatted_output);
                }

                formatted_output = realloc(formatted_output, old_len + 255);
                snprintf(formatted_output+old_len, 254, "%s %.1fF %s\r\n", time_string, forecast_entry->weather_entry.temp, forecast_entry->weather_entry.precipitation);
            }
            return variant_create_string(formatted_output);
        }
        else if(0 == strcmp(range, "Raw") && NULL != weather_cache.raw_forecast)
        {
            return variant_create_string(strdup(weather_cache.raw_forecast));
        }
        else
        {
            return variant_create_string(strdup("no data"));
        }
    }
    return NULL;
}
