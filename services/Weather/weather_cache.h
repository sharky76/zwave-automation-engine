#include "stack.h"
#include <time.h>

typedef struct weather_cache_t
{
    int cache_age;

    double  temp;
    double  windspeed;
    char*   precipitation;
    int     humidity;
    char*   raw_forecast;
    variant_stack_t* forecast_hourly;
} weather_cache_t;

typedef struct weather_forecast_cache_t
{
    time_t timestamp;
    weather_cache_t weather_entry;
} weather_forecast_cache_t;

extern weather_cache_t  weather_cache;

void    weather_cache_init();
void    weather_cache_refresh();
