#include "event_dispatcher.h"

#define MAX_EVENT_HANDLERS 128
#define MAX_QUEUE_SIZE 1000

void event_pump_init(event_pump_t* pump);
void event_pump_send_event(event_pump_t* pump, int event_id, void* event_data);

void event_pump_register_handler(event_pump_t* pump, ...);
void event_pump_unregister_handler(event_pump_t* pump, ...);

void event_pump_poll(event_pump_t* pump, struct timespec* ts);
