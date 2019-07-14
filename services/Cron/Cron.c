#include <service.h>
#include "cron_config.h"
#include "cron_cli.h"
#include <logger.h>
#include <time.h>
#include "crontab.h"
#include "event_dispatcher.h"

int DT_CRON;

static int timer_tick_counter = 0;
void  timer_tick_event(event_pump_t* pump, int timer_id, void* context);

void  service_create(service_t** service, int service_id)
{
    SERVICE_INIT(Cron, "Provides cron-like ability to execute scenes or commands at pre defined times");

    DT_CRON = service_id;

    (*service)->get_config_callback = cron_cli_get_config;

    event_pump_t* pump = event_dispatcher_get_pump("TIMER_PUMP");
    int timer_id = event_dispatcher_register_handler(pump, 60*1000, false, timer_tick_event, NULL);
    pump->start(pump, timer_id);

    // Test
    /*
    crontab_time_t ee;
    ADD_MINUTE(ee, 0);
    ADD_HOUR(ee, 6);
    ADD_DAY(ee, -1);
    ADD_MONTH(ee, -1);
    ADD_WEEKDAY(ee, -1);
    crontab_add_entry(ee, "TestScene");

    crontab_time_t ee2;
    ADD_MINUTE(ee2, 0);
    ADD_HOUR(ee2, 6);
    ADD_DAY(ee2, -1);
    ADD_MONTH(ee2, -1);
    ADD_WEEKDAY(ee2, 1);
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
            printf("1 Found scene: %s\n", variant_get_string(scene_variant));
        }

    scene_stack = crontab_get_scene(ee2);

    {
        stack_for_each(scene_stack, scene_variant2)
        {
            printf("2 Found scene: %s\n", variant_get_string(scene_variant2));
        }
    }

    
    crontab_del_entry(ee3, "TestScene33");



    {
    variant_stack_t* scene_stack2 = crontab_get_scene(ee);

    stack_for_each(scene_stack2, scene_variant2)
    {
        printf("3 Found scene: %s\n", variant_get_string(scene_variant2));
    }
    }

    {
    variant_stack_t* scene_stack2 = crontab_get_scene(ee2);

    stack_for_each(scene_stack2, scene_variant2)
    {
        printf("4 Found scene: %s\n", variant_get_string(scene_variant2));
    }
    }


    printf("Del last entry for ee3\n");
    crontab_del_entry(ee3, "TestScene");

    {
    variant_stack_t* scene_stack3 = crontab_get_scene(ee3);

    if(scene_stack3 == NULL || scene_stack3->count == 0)
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
    crontab_del_entry(ee, "TestScene");

    printf("Adding scene to the empty tree:\n");
    //crontab_add_entry(ee, "TestScene");

    variant_stack_t* scene_stack3 = crontab_get_scene(ee3);

    if(NULL == scene_stack3 || scene_stack3->count == 0)
    {
        printf("Scene EE3 not found\n");
    }
    scene_stack3 = crontab_get_scene(ee2);

    if(NULL == scene_stack3 || scene_stack3->count == 0)
    {
        printf("Scene EE2 not found\n");
    }
    else
    {
        stack_for_each(scene_stack3, scene_variant2)
        {
            printf("2 Found scene: %s\n", variant_get_string(scene_variant2));
        }
    }

    scene_stack3 = crontab_get_scene(ee);

    if(NULL != scene_stack3 && scene_stack3->count > 0)
    {
        stack_for_each(scene_stack3, scene_variant2)
        {
            printf("1 Found scene: %s\n", variant_get_string(scene_variant2));
        }
    }
    else
    {
        printf("Scene EE not found\n");
    } 
    */ 

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
void  timer_tick_event(event_pump_t* pump, int timer_id, void* context)
{
    time_t t = time(NULL);
    struct tm* p_tm = localtime(&t);

    current_time_value_t job_time;
    ADD_MINUTE(job_time, p_tm->tm_min);
    ADD_HOUR(job_time, p_tm->tm_hour);
    ADD_DAY(job_time, p_tm->tm_mday);
    ADD_MONTH(job_time, p_tm->tm_mon);
    ADD_WEEKDAY(job_time, p_tm->tm_wday);

    LOG_DEBUG(DT_CRON, "Cron Event %d %d %d %d %d", MINUTE(job_time), HOUR(job_time), DAY(job_time), MONTH(job_time), WEEKDAY(job_time));

    variant_stack_t* action_list = crontab_get_action(job_time);

    if(NULL != action_list)
    {
        LOG_ADVANCED(DT_CRON, "Found %d matching actions", action_list->count);
        stack_for_each(action_list, cron_action_variant)
        {
            cron_action_t* action = VARIANT_GET_PTR(cron_action_t, cron_action_variant);

            if(action->type == CA_SCENE)
            {
                const char* scene_name = variant_get_string(action->command);
                LOG_DEBUG(DT_CRON, "Sending event for scene %s", scene_name);
                service_post_event_new(DT_CRON, SceneActivationEvent, strdup(scene_name));
            }
            else if(action->type == CA_COMMAND)
            {
                const char* command_name = variant_get_string(action->command);
                LOG_DEBUG(DT_CRON, "Executing command %s", command_name);
                //service_post_event(DT_CRON, COMMAND_ACTIVATION_EVENT, variant_create_string(strdup(command_name)));
                service_post_event_new(DT_CRON, CommandActivationEvent, strdup(command_name));
            }
        }

        stack_free(action_list);
    }
}

