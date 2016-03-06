#include <service.h>
#include "cron_config.h"
#include "cron_cli.h"
#include <logger.h>
#include <time.h>

int DT_CRON;
variant_stack_t* crontab;

void  timer_tick_event(const char* name, event_t* pevent);

void  service_create(service_t** service, int service_id)
{
    SERVICE_INIT(Cron, "Provides cron-like ability to execute scenes or commands at pre defined times");
    SERVICE_ADD_EVENT_HANDLER(timer_tick_event);

    DT_CRON = service_id;
}

void    service_cli_create(cli_node_t* parent_node)
{
    cron_cli_init(parent_node);
}

void  timer_tick_event(const char* name, event_t* pevent)
{
    LOG_DEBUG(DT_CRON, "Event %s", name);

    stack_for_each(crontab, entry_variant)
    {
        cron_entry_t* entry = (cron_entry_t*)variant_get_ptr(entry_variant);
        process_crontab_entry(entry, time(NULL));
    }
}

