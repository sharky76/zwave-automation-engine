#include "event_dispatcher.h"
#include "hash.h"

void timer_pump_init(event_pump_t* pump);
void timer_pump_add_timeout(event_pump_t* pump, ...);
void timer_pump_remove_timeout(event_pump_t* pump, ...);
void timer_pump_poll(event_pump_t* pump, struct timespec* ts);

void timer_pump_start_timer(event_pump_t* pump, ...);
void timer_pump_stop_timer(event_pump_t* pump, ...);
bool timer_pump_is_timer_started(event_pump_t* pump, void* handler);