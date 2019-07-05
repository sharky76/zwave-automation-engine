
#include "event_dispatcher.h"
#include <poll.h>

void  http_server_create(int socket, void (*http_event_handler)(event_pump_t*,int,void*));
int socket_create_server(int port, void(*on_connect)(event_pump_t*,int,void*), void* context);
int socket_create_server_old(int port);