#include <service.h>

variant_t*  weather_get_temp(service_method_t* method, va_list args);
variant_t*  weather_get_windspeed(service_method_t* method, va_list args);
variant_t*  weather_get_precipitation(service_method_t* method, va_list args);
variant_t*  weather_refresh_cache(service_method_t* method, va_list args);
variant_t*  weather_get_humidity(service_method_t* method, va_list args);
variant_t*  weather_get_sunrise(service_method_t* method, va_list args);
variant_t*  weather_get_sunset(service_method_t* method, va_list args);
variant_t*  weather_get_forecast(service_method_t* method, va_list args);
