#include "event.h"
#include "event_pump.h"

static uint32_t monotonic_event_id;

event_entry_t* event_create()
{
    event_entry_t* new_entry = calloc(1, sizeof(event_entry_t));
    new_entry->event_id = monotonic_event_id++;
    new_entry->timestamp = time(NULL);

    return new_entry;
}

void delete_event_entry(void* arg)
{
    event_entry_t* event = (event_entry_t*)arg;
    free(event->event_data);
    free(event);
}

void event_send(event_entry_t* event)
{
    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    event_pump_send_event(pump, EventLogNewEvent, (void*)event, &delete_event_entry);
}