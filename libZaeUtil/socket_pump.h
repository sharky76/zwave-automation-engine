#pragma once 

#include <poll.h>
#include <stdbool.h>
#include "event_dispatcher.h"

#define MAX_POLLFDS 256

void socket_pump_init(event_pump_t* pump);
void socket_pump_add_sock(event_pump_t* pump, va_list args);
void socket_pump_remove_sock(event_pump_t* pump, va_list args);
void socket_pump_poll(event_pump_t* pump, struct timespec* ts);
void socket_pump_set_event_flags(event_pump_t* pump, int fd, int flags, bool is_set);