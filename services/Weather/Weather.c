#include <service.h>
#include <logger.h>
#include "weather_methods.h"
#include "weather_cli.h"
#include "weather_cache.h"

char*  weather_country_code;
int    weather_zip;
char*  weather_temp_units;
int    DT_WEATHER;

void   weather_event_handler(const char* sender, event_t* event);

void    service_create(service_t** service, int service_id)
{
    SERVICE_INIT(Weather, "Provide current weather information");
    SERVICE_ADD_METHOD(GetTemperature,      weather_get_temp, 0, "Get current temperature");
    SERVICE_ADD_METHOD(GetWindSpeed,        weather_get_windspeed, 0, "Get current wind speed");
    SERVICE_ADD_METHOD(GetPrecipitation,    weather_get_precipitation, 0, "Get current precipitation");
    SERVICE_ADD_METHOD(GetHumidity,         weather_get_humidity, 0, "Get current humidity");
    SERVICE_ADD_METHOD(GetSunrise,          weather_get_sunrise, 0, "Get sunrise time");
    SERVICE_ADD_METHOD(GetSunset,           weather_get_sunset, 0, "Get sunset time");
    SERVICE_ADD_METHOD(Refresh,             weather_refresh_cache, 0, "Update stored weather information");
    SERVICE_ADD_METHOD(GetForecast,         weather_get_forecast, 1, "Return weather forecast, args: Hourly|Daily|Raw");

    (*service)->get_config_callback = weather_cli_get_config;
    DT_WEATHER = service_id;

    weather_cache_init();
}

void    service_cli_create(cli_node_t* parent_node)
{
    weather_cli_init(parent_node);
}


