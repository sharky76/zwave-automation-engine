#include "weather_cli.h"
#include "weather_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char** config_list = NULL;

bool    weather_cli_set_location(vty_t* vty, variant_stack_t* params);
bool    weather_cli_set_metric(vty_t* vty, variant_stack_t* params);
bool    weather_cli_set_imperial(vty_t* vty, variant_stack_t* params);

cli_node_t* weather_node;

cli_command_t    weather_command_list[] = {
    {"location country-code WORD zip INT",   &weather_cli_set_location,  "Set location. Country code is 2-letter code (us, ca, etc...)"},
    {"units metric", &weather_cli_set_metric, "Set metric units"},
    {"units imperial", &weather_cli_set_imperial, "Set imperial units"},
    {NULL, NULL, NULL}
};

void    weather_cli_init(cli_node_t* parent_node)
{
    cli_install_node(&weather_node, parent_node, weather_command_list, "Weather", "service-weather");
    weather_temp_units = strdup("imperial");
    weather_country_code = strdup("us");
    weather_zip = 0;
}

char**  weather_cli_get_config(vty_t* vty)
{
    if(NULL != config_list)
    {
        char* cfg;
        int i = 0;
        while(cfg = config_list[i++])
        {
            free(cfg);
        }

        free(config_list);
    }

    config_list = calloc(3, sizeof(char*));
    
    char buf[128] = {0};
    snprintf(buf, 127, "location country-code %s zip %d", weather_country_code, weather_zip);
    config_list[0] = strdup(buf);
    memset(buf, 0, 127);
    snprintf(buf, 31, "units %s", weather_temp_units);
    config_list[1] = strdup(buf);
    return config_list;
}

bool    weather_cli_set_location(vty_t* vty, variant_stack_t* params)
{
    free(weather_country_code);
    weather_country_code = strdup(variant_get_string(stack_peek_at(params, 2)));
    weather_zip = variant_get_int(stack_peek_at(params, 4));
}

bool    weather_cli_set_metric(vty_t* vty, variant_stack_t* params)
{
    free(weather_temp_units);
    weather_temp_units = strdup("metric");
}

bool    weather_cli_set_imperial(vty_t* vty, variant_stack_t* params)
{
    free(weather_temp_units);
    weather_temp_units = strdup("imperial");
}

