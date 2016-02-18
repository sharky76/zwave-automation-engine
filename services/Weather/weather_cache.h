typedef struct weather_cache_t
{
    int cache_age;

    double  temp;
    double  windspeed;
    char*   precipitation;
    int     humidity;
} weather_cache_t;

extern weather_cache_t  weather_cache;

void    weather_cache_init();
void    weather_cache_refresh();
