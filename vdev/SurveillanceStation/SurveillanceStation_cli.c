#include "SurveillanceStation_cli.h"
#include "SS_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "SS_api.h"

char** config_list = NULL;

cli_node_t* SS_node;
bool    cmd_set_auth(vty_t* vty, variant_stack_t* params);
bool    cmd_set_url(vty_t* vty, variant_stack_t* params);

cli_command_t    SS_command_list[] = {
    {"username WORD password WORD",       cmd_set_auth,   "Set SurveillanceStation authentication"},
    {"base-url WORD",                          cmd_set_url,    "Set SurveillanceStation base URL"},
    {NULL,  NULL,   NULL}
};

void SurveillanceStation_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&SS_node, parent_node, SS_command_list, "SurveillanceStation", "vdev-SurveillanceStation");
}

char** SurveillanceStation_get_config(vty_t* vty)
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

    int param_line_count = 2;
    if(NULL == SS_user || NULL == SS_pass)
    {
        param_line_count--;
    }
        
    if(NULL == SS_base_url)
    {
        param_line_count--;
    }

    config_list = calloc(param_line_count+1, sizeof(char*));

    if(NULL != SS_user && NULL != SS_pass)
    {
        char buf[512] = {0};
        snprintf(buf, 511, "username %s password %s", SS_user, SS_pass);
        config_list[config_index++] = strdup(buf);
    }

    if(NULL != SS_base_url)
    {
        char buf[512] = {0};
        snprintf(buf, 511, "base-url %s", SS_base_url);
        config_list[config_index++] = strdup(buf);
    }
    return config_list;
}

bool    cmd_set_auth(vty_t* vty, variant_stack_t* params)
{
    free(SS_user);
    free(SS_pass);

    SS_user = strdup(variant_get_string(stack_peek_at(params, 1)));
    SS_pass = strdup(variant_get_string(stack_peek_at(params, 3)));

    if(SS_device_started)
    {
        SS_api_logout();
        SS_api_query();
        SS_api_get_sid();
        SS_api_get_info();
    }
}

bool    cmd_set_url(vty_t* vty, variant_stack_t* params)
{
    free(SS_base_url);
    SS_base_url = strdup(variant_get_string(stack_peek_at(params, 1)));

    if(SS_device_started)
    {
        SS_api_logout();
        SS_api_query();
        SS_api_get_sid();
        SS_api_get_info();
    }
}
