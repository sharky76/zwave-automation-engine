#include "cron_cli.h"
#include "crontab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool cmd_cron_add_entry(vty_t* vty, variant_stack_t* params);
bool cmd_cron_del_entry(vty_t* vty, variant_stack_t* params);

cli_node_t* cron_node;

cli_command_t cron_command_list[] = {
    {"job *|INT[0-59] *|INT[0-23] *|INT[1-31] *|INT[1-12] *|INT[0-6]|LIST[0-6] scene|command LINE",   cmd_cron_add_entry, "Add cron entry: min[0-59] hour[0-23] day of month[1-31] month[1-12] day of week[0-6] (0 = Sunday). * means first-last"},
    {"no job *|INT[0-59] *|INT[0-23] *|INT[1-31] *|INT[1-12] *|INT[0-6]|LIST[0-6] scene|command LINE",   cmd_cron_del_entry, "Add cron entry: min[0-59] hour[0-23] day of month[1-31] month[1-12] day of week[0-6] (0 = Sunday). * means first-last"},
    {NULL, NULL, NULL}
};

typedef struct cron_cli_record_t
{
    crontab_time_t time;
    CronAction type;
    char* command;
} cron_cli_record_t;

variant_stack_t*    cron_cli_record_list;
char** config_list;

void delete_cron_cli_record(void* arg)
{
    cron_cli_record_t* r = (cron_cli_record_t*)arg;

    for(int i = 0; i < 5; i++)
    {
        stack_free(r->time[i]);
    }
    free(r->command);
    free(r);
}

void cron_cli_init(cli_node_t* parent_node)
{
    cli_install_node(&cron_node, parent_node, cron_command_list, "Cron", "service-cron");
    cron_cli_record_list = stack_create();
}

char* crontab_time_to_string(crontab_time_t time)
{
    char* time_str = calloc(128, sizeof(char));
    char* time_ptr = time_str;
    for(int i = 0; i < 5; i++)
    {
        variant_stack_t* time_list = time[i];

        int time_count = time_list->count;
        stack_for_each(time_list, time_value_variant)
        {
            int time_value = variant_get_int(time_value_variant);
            if(time_value == -1)
            {
                strcat(time_ptr++, "*");
            }
            else
            {
                int n = sprintf(time_ptr, "%d%s", time_value, (--time_count > 0)? "," : "");
                time_ptr += n;
            }
        }

        if(i < 4)
        {
            strcat(time_ptr++, " ");
        }
    }

    return time_str;
}

char** cron_cli_get_config(vty_t* vty)
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

    config_list = calloc(cron_cli_record_list->count+1, sizeof(char*));

    int i = 0;
    stack_for_each(cron_cli_record_list, crontab_record_variant)
    {
        config_list[i] = calloc(512, sizeof(char));
        cron_cli_record_t* crontab_record = (cron_cli_record_t*)variant_get_ptr(crontab_record_variant);
        char* crontab_time = crontab_time_to_string(crontab_record->time);
        snprintf(config_list[i], 511, "job %s %s %s", crontab_time, (crontab_record->type == CA_SCENE)? "scene" : "command", crontab_record->command);
        free(crontab_time);
        i++;
    }

    return config_list;
}

bool cmd_cron_add_entry(vty_t* vty, variant_stack_t* params)
{
    cron_cli_record_t* crontab_record = malloc(/*sizeof(char*) + sizeof(int)*5*/sizeof(cron_cli_record_t));

    for(int i = 0; i < 5; i++)
    {
        crontab_record->time[i] = stack_create();
    }

    // Process command line and add more or modify existing time entry
    for(int i = 1; i < 6; i++)
    {
        variant_t* time_val = stack_peek_at(params, i);
        if(time_val->type == DT_STRING && strcmp(variant_get_string(time_val), "*") == 0)
        {
            stack_push_back(crontab_record->time[i-1], variant_create_int32(DT_INT32, -1));
        }
        else if(time_val->type == DT_INT32)
        {
            stack_push_back(crontab_record->time[i-1], variant_clone(time_val));
        }
        else if(time_val->type == DT_LIST)
        {
            variant_stack_t* int_list = variant_get_list(time_val);

            stack_for_each(int_list, time_value_variant)
            {
                stack_push_back(crontab_record->time[i-1], variant_clone(time_value_variant));
            }
        }
    }

    variant_t* action_type = stack_peek_at(params, 6);
    variant_t* command = stack_peek_at(params, 7);

    if(strcmp(variant_get_string(action_type), "scene") == 0)
    {
        crontab_record->type = CA_SCENE;
    }
    else
    {
        crontab_record->type = CA_COMMAND;
    }


    crontab_record->command = strdup(variant_get_string(command));

    crontab_add_entry(crontab_record->time, crontab_record->type, crontab_record->command);
    stack_push_back(cron_cli_record_list, variant_create_ptr(DT_PTR, crontab_record, &delete_cron_cli_record));
}

bool cmd_cron_del_entry(vty_t* vty, variant_stack_t* params)
{
    cron_cli_record_t crontab_record;
    for(int i = 0; i < 5; i++)
    {
        crontab_record.time[i] = stack_create();
    }

    for(int i = 2; i < 7; i++)
    {
        variant_t* time_val = stack_peek_at(params, i);
        if(time_val->type == DT_STRING && strcmp(variant_get_string(time_val), "*") == 0)
        {
            stack_push_back(crontab_record.time[i-2], variant_create_int32(DT_INT32, -1));
        }
        else if(time_val->type == DT_INT32)
        {
            stack_push_back(crontab_record.time[i-2], variant_clone(time_val));
        }
        else if(time_val->type == DT_LIST)
        {
            variant_stack_t* int_list = variant_get_list(time_val);

            stack_for_each(int_list, time_value_variant)
            {
                stack_push_back(crontab_record.time[i-2], variant_clone(time_value_variant));
            }
        }
    }

    variant_t* action_type = stack_peek_at(params, 7);
    variant_t* command = stack_peek_at(params, 8);

    if(strcmp(variant_get_string(action_type), "scene") == 0)
    {
        crontab_record.type = CA_SCENE;
    }
    else
    {
        crontab_record.type = CA_COMMAND;
    }
    crontab_record.command = (char*)variant_get_string(command);

    stack_for_each(cron_cli_record_list, crontab_record_variant)
    {
        cron_cli_record_t* saved_record = (cron_cli_record_t*)variant_get_ptr(crontab_record_variant);
        if(/*memcmp(saved_record->time, crontab_record.time, sizeof(int)*5) == 0*/crontab_time_compare(saved_record->time, crontab_record.time))
        {
            if(strcmp(saved_record->command, crontab_record.command) == 0 && saved_record->type == crontab_record.type)
            {
                crontab_del_entry(saved_record->time, saved_record->type, saved_record->command);
                stack_remove(cron_cli_record_list, crontab_record_variant);
                variant_free(crontab_record_variant);
                break;
            }
        }
    }

    for(int i = 0; i < 5; i++)
    {
        stack_free(crontab_record.time[i]);
    }
}

