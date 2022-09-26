#include "event.h"
#include "event_pump.h"
#include <allocator.h>

static uint32_t monotonic_event_id;
static pool_t* event_allocator_pool = NULL;

void event_pool_init()
{
    event_allocator_pool = const_allocator_init(sizeof(event_entry_t), 50);
}

event_entry_t* event_create()
{
    // event_entry_t* new_entry = calloc(1, sizeof(event_entry_t));
    event_entry_t* new_entry = const_allocator_new(event_allocator_pool);
    new_entry->event_id = monotonic_event_id++;
    new_entry->timestamp = time(NULL);

    return new_entry;
}

void delete_event_entry(void* arg)
{
    event_entry_t* event = (event_entry_t*)arg;
    free(event->event_data);
    const_allocator_delete(event);
}

void event_send(event_entry_t* event)
{
    event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
    event_pump_send_event(pump, EventLogNewEvent, (void*)event, &delete_event_entry);
}