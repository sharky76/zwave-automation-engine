#include <service.h>
#include "cron_config.h"
#include "cron_cli.h"
#include <logger.h>
#include <time.h>
#include "crontab.h"

int DT_CRON;

static int timer_tick_counter;
void  timer_tick_event(const char* name, event_t* pevent);

void  service_create(service_t** service, int service_id)
{
    SERVICE_INIT(Cron, "Provides cron-like ability to execute scenes or commands at pre defined times");

    DT_CRON = service_id;

    (*service)->get_config_callback = cron_cli_get_config;
    SERVICE_SUBSCRIBE_TO_EVENT_SOURCE("Timer", timer_tick_event);

    // Test
    /*crontab_time_t ee;
    ADD_MINUTE(ee, 1);
    ADD_HOUR(ee, 4);
    ADD_DAY(ee, 6);
    ADD_MONTH(ee, 8);
    ADD_WEEKDAY(ee, 7);
    crontab_add_entry(ee, "TestScene");

    crontab_time_t ee2;
    ADD_MINUTE(ee2, 1);
    ADD_HOUR(ee2, 4);
    ADD_DAY(ee2, 6);
    ADD_MONTH(ee2, 8);
    ADD_WEEKDAY(ee2, 2);
    crontab_add_entry(ee2, "TestScene2");

    crontab_time_t ee3;
    ADD_MINUTE(ee3, 1);
    ADD_HOUR(ee3, 4);
    ADD_DAY(ee3, 6);
    ADD_MONTH(ee3, 8);
    ADD_WEEKDAY(ee3, 7);
    crontab_add_entry(ee3, "TestScene33");

    variant_stack_t* scene_stack = crontab_get_scene(ee);

    stack_for_each(scene_stack, scene_variant)
    {
        printf("Found scene: %s\n", variant_get_string(scene_variant));
    }

    crontab_del_entry(ee3, "TestScene33");



    {
    variant_stack_t* scene_stack2 = crontab_get_scene(ee);

    stack_for_each(scene_stack2, scene_variant2)
    {
        printf("Found scene: %s\n", variant_get_string(scene_variant2));
    }
    }

    {
    variant_stack_t* scene_stack2 = crontab_get_scene(ee2);

    stack_for_each(scene_stack2, scene_variant2)
    {
        printf("Found scene: %s\n", variant_get_string(scene_variant2));
    }
    }


    printf("Del last entry for ee3\n");
    crontab_del_entry(ee3, "TestScene");

    {
    variant_stack_t* scene_stack3 = crontab_get_scene(ee3);

    if(scene_stack3 == NULL)
    {
        printf("NOT FOUND ee3\n");
    }
    else
    {
        stack_for_each(scene_stack3, scene_variant2)
        {
            printf("Found scene: %s\n", variant_get_string(scene_variant2));
        }
    }
    }

    crontab_del_entry(ee2, "TestScene2");

    printf("Adding scene to the empty tree:\n");
    crontab_add_entry(ee, "TestScene");*/

}

void    service_cli_create(cli_node_t* parent_node)
{
    cron_cli_init(parent_node);
}

/**
 * Timer emits "tick" events every second
 * 
 * @author alex (3/8/2016)
 * 
 * @param name 
 * @param pevent 
 */
void  timer_tick_event(const char* name, event_t* pevent)
{
    if(++timer_tick_counter > 60 && strcmp(variant_get_string(pevent->data), "tick") == 0)
    {
        timer_tick_counter = 0;
        LOG_DEBUG(DT_CRON, "Event %s tick", name);
    
        time_t t = time(NULL);
        struct tm* p_tm = localtime(&t);
    
        crontab_time_t job_time;
        ADD_MINUTE(job_time, p_tm->tm_min);
        ADD_HOUR(job_time, p_tm->tm_hour);
        ADD_DAY(job_time, p_tm->tm_mday);
        ADD_MONTH(job_time, p_tm->tm_mon);
        ADD_WEEKDAY(job_time, p_tm->tm_wday);

        variant_stack_t* scene_list = crontab_get_scene(job_time);

        if(NULL != scene_list)
        {
            stack_for_each(scene_list, scene_variant)
            {
                LOG_DEBUG(DT_CRON, "Sending event for scene %s", variant_get_string(scene_variant));
                service_post_event(DT_CRON, variant_get_string(scene_variant));
            }
        }
    }
}

