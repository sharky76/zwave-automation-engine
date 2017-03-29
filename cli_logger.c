#include "cli_logger.h"
#include <string.h>
#include <stdlib.h>

cli_node_t*     logger_node;
logger_service_t** logger_services;
static int logger_services_size;

bool    cmd_enable_logger(vty_t* vty, variant_stack_t* params);
bool    cmd_disable_logger(vty_t* vty, variant_stack_t* params);
bool    cmd_set_log_level(vty_t* vty, variant_stack_t* params);
bool    cmd_del_log_level(vty_t* vty, variant_stack_t* params);
bool    cmd_show_logger(vty_t* vty, variant_stack_t* params);
bool    cmd_set_log_history(vty_t* vty, variant_stack_t* params);
bool    cmd_del_log_history(vty_t* vty, variant_stack_t* params);
bool    cmd_enable_console(vty_t* vty, variant_stack_t* params);
bool    cmd_disable_console(vty_t* vty, variant_stack_t* params);
bool    cmd_show_logger_history(vty_t* vty, variant_stack_t* params);
bool    cmd_clear_logger_history(vty_t* vty, variant_stack_t* params);

cli_command_t   logger_root_list[] = {
    {"logging enable",                           cmd_enable_logger,  "Enable logger"},
    {"logging history INT",                      cmd_set_log_history, "Set log history size"},
    {"no logging history",                       cmd_del_log_history, "Disable log history"},
    {"no logging enable",                        cmd_disable_logger, "Disable logger"},
    {"logging console",                          cmd_enable_console, "Enable console logging"},
    {"no logging console",                       cmd_disable_console, "Disable console logging"},
    {"show logging",                             cmd_show_logger,    "Show logger configuration"},
    {"show logging history",                     cmd_show_logger_history, "Show logger history"},
    {"clear logging",                            cmd_clear_logger_history, "Clear logger history"},
    {NULL,                     NULL,                            NULL}
};

char full_command[1024];
char full_no_command[1024];
cli_node_t* logger_parent_node;

cli_command_t   logger_service_list[] = {
        {full_command,      cmd_set_log_level, "Set logger level per class"},
        {full_no_command,   cmd_del_log_level, "Remove logging class"},
        {NULL,   NULL,   NULL}
    };

void    cli_logger_init(cli_node_t* parent_node)
{
    logger_services = NULL;
    cli_append_to_node(parent_node, logger_root_list);

    // Create custom command with all registered logging modules...
    int logger_services_size;
    char class_buffer[512] = {0};
    if(logger_get_services(&logger_services, &logger_services_size))
    {
        for(int i = 0; i < logger_services_size; i++)
        {
            logger_service_t* s = *(logger_services + i);
            strcat(class_buffer, s->service_name);

            if(i < logger_services_size-1)
            {
                strcat(class_buffer, "|");
            }
        }

    }

    snprintf(full_command, 1023, "logging class %s error|info|advanced|debug", class_buffer);
    snprintf(full_no_command, 1023, "no logging class %s", class_buffer);

    cli_append_to_node(parent_node, logger_service_list);
    logger_parent_node = parent_node;
}

void    cli_add_logging_class(const char* class_name)
{
    char* full_command = calloc(1024, sizeof(char));
    char* full_no_command = calloc(1024, sizeof(char));
    snprintf(full_command, 1023, "logging class %s error|info|advanced|debug", class_name);     
    snprintf(full_no_command, 1023, "no logging class %s", class_name);                         

    cli_command_t* command_list = calloc(3, sizeof(cli_command_t));
    command_list[0].name = full_command;
    command_list[0].func=cmd_set_log_level;
    command_list[0].help=strdup("Set logger level per class");

    command_list[1].name=full_no_command;
    command_list[1].func=cmd_del_log_level;
    command_list[1].help=strdup("Remove logging class");
    cli_append_to_node(logger_parent_node, command_list);
    
    if(NULL != logger_services)
    {
        free(logger_services);
    }

    logger_get_services(&logger_services, &logger_services_size);                                  
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
    const char* log_class = variant_get_string(stack_peek_at(params, 2));
    const char* log_level = variant_get_string(stack_peek_at(params, 3));

    int service_id;

    for(int i = 0; i < logger_services_size; i++)
    {
        logger_service_t* s = *(logger_services + i);
        if(strcmp(s->service_name, log_class) == 0)
        {
            service_id = s->service_id;
            break;
        }
    }

    if(strcmp(log_level, "info") == 0)
    {
        logger_set_level(service_id, LOG_LEVEL_BASIC);
    }
    else if(strcmp(log_level, "error") == 0)
    {
        logger_set_level(service_id, LOG_LEVEL_ERROR);
    }
    else if(strcmp(log_level, "advanced") == 0)
    {
        logger_set_level(service_id, LOG_LEVEL_ADVANCED);
    }
    else if(strcmp(log_level, "debug") == 0)
    {
        logger_set_level(service_id, LOG_LEVEL_DEBUG);
    }
}

bool    cmd_del_log_level(vty_t* vty, variant_stack_t* params)
{
    const char* log_class = variant_get_string(stack_peek_at(params, 3));

    int service_id;
    for(int i = 0; i < logger_services_size; i++)
    {
        logger_service_t* s = *(logger_services + i);
        if(strcmp(s->service_name, log_class) == 0)
        {
            service_id = s->service_id;
            break;
        }
    }

    logger_disable_service(service_id);
}

bool    cmd_set_log_history(vty_t* vty, variant_stack_t* params)
{
    logger_set_buffer(variant_get_int(stack_peek_at(params, 2)));
}

bool    cmd_del_log_history(vty_t* vty, variant_stack_t* params)
{
    logger_set_buffer(0);
}

bool    cmd_enable_console(vty_t* vty, variant_stack_t* params)
{
    logger_set_online(vty, true);
    vty_write(vty, "%% Console logging enabled%s", VTY_NEWLINE(vty));
}

bool    cmd_disable_console(vty_t* vty, variant_stack_t* params)
{
    logger_set_online(vty, false);
    vty_write(vty, "%% Console logging disabled%s", VTY_NEWLINE(vty));
}

bool    cmd_show_logger_history(vty_t* vty, variant_stack_t* params)
{
    logger_print_buffer();
}

bool    cmd_show_logger(vty_t* vty, variant_stack_t* params)
{
    if(logger_is_enabled())
    {
        vty_write(vty, "logging enable%s", VTY_NEWLINE(vty));
    }
    else
    {
        vty_write(vty, "no logging enable%s", VTY_NEWLINE(vty));
    }

    int hist_size = logger_get_buffer_size();
    if(hist_size > 0)
    {
        vty_write(vty, "logging history %d%s", hist_size, VTY_NEWLINE(vty));
    }
    else
    {
        vty_write(vty, "no logging history%s", VTY_NEWLINE(vty));
    }

    for(int i = 0; i < logger_services_size; i++)
    {
        logger_service_t* s = *(logger_services + i);
        if(s->enabled)
        {
            switch(logger_get_level(s->service_id))
            {
            case LOG_LEVEL_BASIC:
                vty_write(vty, "logging class %s info%s", s->service_name, VTY_NEWLINE(vty));
                break;
            case LOG_LEVEL_ERROR:
                vty_write(vty, "logging class %s error%s", s->service_name, VTY_NEWLINE(vty));
                break;
            case LOG_LEVEL_ADVANCED:
                vty_write(vty, "logging class %s advanced%s", s->service_name, VTY_NEWLINE(vty));
                break;
            case LOG_LEVEL_DEBUG:
                vty_write(vty, "logging class %s debug%s", s->service_name, VTY_NEWLINE(vty));
                break;
            }
        }
    }
}

bool    cmd_clear_logger_history(vty_t* vty, variant_stack_t* params)
{
    logger_clear_buffer();
}
