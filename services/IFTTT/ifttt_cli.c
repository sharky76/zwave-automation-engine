#include "ifttt_cli.h"
#include "vty.h"
#include "ifttt_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char** config_list = NULL;

cli_node_t* ifttt_node;

bool    cmd_configure_key(vty_t* vty, variant_stack_t* params);

cli_command_t    ifttt_command_list[] = {
    {"key WORD",       cmd_configure_key,   "Set IFTTT Maker key"},
    //{"url WORD scene|command|script LINE",       cmd_configure_trigger,   "Add IFTTT trigger"},
    {NULL, NULL, NULL}
};

void   ifttt_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&ifttt_node, parent_node, ifttt_command_list, "IFTTT", "service-IFTTT");
}

char** ifttt_cli_get_config(vty_t* vty)
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

    int config_index = 0;
    config_list = calloc(2, sizeof(char*));

    if(NULL != ifttt_key)
    {
        char buf[512] = {0};
        snprintf(buf, 511, "key %s", ifttt_key);
        config_list[0] = strdup(buf);
    }

    return config_list;
}

bool    cmd_configure_key(vty_t* vty, variant_stack_t* params)
{
    const char* key = variant_get_string(stack_peek_at(params, 1));
    
    if(NULL != ifttt_key)
    {
        free(ifttt_key);
    }

    ifttt_key = strdup(key);
    return true;
}
