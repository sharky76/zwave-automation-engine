#include "cron_cli.h"
#include "crontab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool cmd_cron_add_entry(vty_t* vty, variant_stack_t* params);
bool cmd_cron_del_entry(vty_t* vty, variant_stack_t* params);

cli_node_t* cron_node;

cli_command_t cron_command_list[] = {
    {"job *|INT[0-59] *|INT[0-23] *|INT[1-31] *|INT[1-12] *|INT[0-7] scene LINE",   cmd_cron_add_entry, "Add cron entry: min[0-59] hour[0-23] day of month[1-31] month[1-12] day of week[0-7] (0 = Monday). * means first-last"},
    {"no job *|INT[0-59] *|INT[0-23] *|INT[1-31] *|INT[1-12] *|INT[0-7] scene LINE",   cmd_cron_del_entry, "Add cron entry: min[0-59] hour[0-23] day of month[1-31] month[1-12] day of week[0-7] (0 = Monday). * means first-last"},
};

typedef struct cron_cli_record_t
{
    crontab_time_t time;
    char* scene;
} cron_cli_record_t;

variant_stack_t*    cron_cli_record_list;
char** config_list;

void delete_cron_cli_record(void* arg)
{
    cron_cli_record_t* r = (cron_cli_record_t*)arg;
    free(r->scene);
    free(r);
}

void cron_cli_init(cli_node_t* parent_node)
{
    cli_install_node(&cron_node, parent_node, cron_command_list, "Cron", "service-cron");
    cron_cli_record_list = stack_create();
}

char* crontab_time_to_string(crontab_time_t time)
{
    char* time_str = calloc(16, sizeof(char));
    char* time_ptr = time_str;
    for(int i = 0; i < 5; i++)
    {
        if(time[i] == -1)
        {
            strcat(time_ptr++, "*");
        }
        else
        {
            int n = sprintf(time_ptr, "%d", time[i]);
            time_ptr += n;
        }

        if(i < 4)
        {
            strcat(time_ptr++, " ");
        }
    }

    return time_str;
}

char** cron_cli_get_config()
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

    config_list = calloc(cron_cli_record_list->count, sizeof(char*));

    int i = 0;
    stack_for_each(cron_cli_record_list, crontab_record_variant)
    {
        config_list[i] = calloc(512, sizeof(char));
        cron_cli_record_t* crontab_record = (cron_cli_record_t*)variant_get_ptr(crontab_record_variant);
        snprintf(config_list[i], 511, "job %s scene %s", crontab_time_to_string(crontab_record->time), crontab_record->scene);
        i++;
    }

    return config_list;
}

bool cmd_cron_add_entry(vty_t* vty, variant_stack_t* params)
{
    cron_cli_record_t* crontab_record = malloc(sizeof(char*) + sizeof(int)*5);

    for(int i = 1; i < 6; i++)
    {
        variant_t* time_val = stack_peek_at(params, i);
        if(time_val->type == DT_STRING && strcmp(variant_get_string(time_val), "*") == 0)
        {
            crontab_record->time[i-1] = -1;
        }
        else
        {
            crontab_record->time[i-1] = variant_get_int(time_val);
        }
    }

    crontab_record->scene = strdup(variant_get_string(stack_peek_at(params, 7)));

    crontab_add_entry(crontab_record->time, crontab_record->scene);
    stack_push_back(cron_cli_record_list, variant_create_ptr(DT_PTR, crontab_record, &delete_cron_cli_record));
}

bool cmd_cron_del_entry(vty_t* vty, variant_stack_t* params)
{
    cron_cli_record_t crontab_record;

    for(int i = 2; i < 7; i++)
    {
        variant_t* time_val = stack_peek_at(params, i);
        if(time_val->type == DT_STRING && strcmp(variant_get_string(time_val), "*") == 0)
        {
            crontab_record.time[i-2] = -1;
        }
        else
        {
            crontab_record.time[i-2] = variant_get_int(time_val);
        }
    }

    crontab_record.scene = (char*)variant_get_string(stack_peek_at(params, 8));

    stack_for_each(cron_cli_record_list, crontab_record_variant)
    {
        cron_cli_record_t* saved_record = (cron_cli_record_t*)variant_get_ptr(crontab_record_variant);
        if(memcmp(saved_record->time, crontab_record.time, sizeof(int)*5) == 0)
        {
            if(strcmp(saved_record->scene, crontab_record.scene) == 0)
            {
                crontab_del_entry(saved_record->time, saved_record->scene);
                stack_remove(cron_cli_record_list, crontab_record_variant);
                variant_free(crontab_record_variant);
                break;
            }
        }
    }
}

