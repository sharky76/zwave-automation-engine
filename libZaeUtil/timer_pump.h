#pragma once 

#include "event_dispatcher.h"

#define MAX_TIMERS  128

void timer_pump_init(event_pump_t* pump);
void timer_pump_free(event_pump_t* pump);
int timer_pump_add_timeout(event_pump_t* pump, va_list args);
void timer_pump_remove_timeout(event_pump_t* pump, va_list args);
void timer_pump_poll(event_pump_t* pump, struct timespec* ts);

void timer_pump_start_timer(event_pump_t* pump, ...);
void timer_pump_stop_timer(event_pump_t* pump, ...);
bool timer_pump_is_singleshot(event_pump_t* pump, int timer_id);
