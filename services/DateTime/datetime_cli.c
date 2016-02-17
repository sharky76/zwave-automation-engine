#include "datetime_cli.h"
#include <stdlib.h>
#include <string.h>

bool    cmd_datetime_format(vty_t* vty, variant_stack_t* params);

char*    datetime_time_format = NULL;
char** config_list;
cli_node_t* datetime_node;

cli_command_t   datetime_command_list[] = {
    {"format WORD",   cmd_datetime_format, "Set time format used when converting time to string"},
    {NULL,       NULL,                NULL}
};

void    datetime_cli_init(cli_node_t* parent_node)
{
    cli_install_node(&datetime_node, parent_node, datetime_command_list, "DateTime", "service-datetime");
}

char**  datetime_cli_get_config()
{
    if(NULL != config_list)
    {
        char* cfg = NULL;
        int i = 0;
        while(cfg = config_list[i++])
        {
            free(cfg);
        }

        free(config_list);
    }

    config_list = calloc(2, sizeof(char*));
    config_list[0] = calloc(strlen(datetime_time_format) + strlen("format ") + 1, sizeof(char));
    config_list[1] = NULL;
    strcat(config_list[0], "format ");
    strcat(config_list[0], datetime_time_format);

    return config_list;
}

bool    cmd_datetime_format(vty_t* vty, variant_stack_t* params)
{
    const char* fmt = variant_get_string(stack_peek_at(params, 1));

    free(datetime_time_format);
    datetime_time_format = strdup(fmt);
}

