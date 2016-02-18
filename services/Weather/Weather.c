#include <service.h>
#include "weather_methods.h"
#include "weather_cli.h"
#include "weather_cache.h"

char*  weather_country_code;
int    weather_zip;
char*  weather_temp_units;

void    service_create(service_t** service, int service_id)
{
    SERVICE_INIT(Weather, "Provide current weather information");
    SERVICE_ADD_METHOD(GetTemperature,      weather_get_temp, 0, "Get current temperature");
    SERVICE_ADD_METHOD(GetWindSpeed,        weather_get_windspeed, 0, "Get current wind speed");
    SERVICE_ADD_METHOD(GetPrecipitation,    weather_get_precipitation, 0, "Get current precipitation");
    SERVICE_ADD_METHOD(Refresh,             weather_refresh_cache, 0, "Update stored weather information");

    (*service)->get_config_callback = weather_cli_get_config;

    weather_cache_init();
}

void    service_cli_create(cli_node_t* parent_node)
{
    weather_cli_init(parent_node);
}
