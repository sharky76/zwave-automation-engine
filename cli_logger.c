#include "cli_logger.h"
#include <string.h>

cli_node_t*     logger_node;

bool    cmd_enable_logger(vty_t* vty, variant_stack_t* params);
bool    cmd_disable_logger(vty_t* vty, variant_stack_t* params);
bool    cmd_set_log_level(vty_t* vty, variant_stack_t* params);
bool    cmd_show_logger(vty_t* vty, variant_stack_t* params);

cli_command_t   logger_root_list[] = {
    {"logger enable",                           cmd_enable_logger,  "Enable logger"},
    {"no logger enable",                        cmd_disable_logger, "Disable logger"},
    {"logger set-level none|error|info|advanced|debug",  cmd_set_log_level,  "Set logger level"},
    {"show logger",                             cmd_show_logger,    "Show logger configuration"},
    {NULL,                     NULL,                            NULL}
};

void    cli_logger_init(cli_node_t* parent_node)
{
    cli_append_to_node(parent_node, logger_root_list);
}

bool    cmd_enable_logger(vty_t* vty, variant_stack_t* params)
{
    logger_enable(true);
}

bool    cmd_disable_logger(vty_t* vty, variant_stack_t* params)
{
    logger_enable(false);
}

bool    cmd_set_log_level(vty_t* vty, variant_stack_t* params)
{
    const char* log_level = variant_get_string(stack_peek_at(params, 2));

    if(strcmp(log_level, "none") == 0)
    {
        logger_set_level(LOG_LEVEL_NONE);
    }
    else if(strcmp(log_level, "info") == 0)
    {
        logger_set_level(LOG_LEVEL_BASIC);
    }
    else if(strcmp(log_level, "error") == 0)
    {
        logger_set_level(LOG_LEVEL_ERROR);
    }
    else if(strcmp(log_level, "advanced") == 0)
    {
        logger_set_level(LOG_LEVEL_ADVANCED);
    }
    else if(strcmp(log_level, "debug") == 0)
    {
        logger_set_level(LOG_LEVEL_DEBUG);
    }
}

bool    cmd_show_logger(vty_t* vty, variant_stack_t* params)
{
    if(logger_is_enabled())
    {
        vty_write(vty, "logger enable\n");
    }
    else
    {
        vty_write(vty, "no logger enable\n");
    }

    switch(logger_get_level())
    {
    case LOG_LEVEL_NONE:
        vty_write(vty, "loger set-level none\n");
        break;
    case LOG_LEVEL_BASIC:
        vty_write(vty, "loger set-level info\n");
        break;
    case LOG_LEVEL_ERROR:
        vty_write(vty, "loger set-level error\n");
        break;
    case LOG_LEVEL_ADVANCED:
        vty_write(vty, "loger set-level advanced\n");
        break;
    case LOG_LEVEL_DEBUG:
        vty_write(vty, "loger set-level debug\n");
        break;
    }
}

