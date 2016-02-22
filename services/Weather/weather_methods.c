#include "weather_methods.h"
#include "weather_cache.h"

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

