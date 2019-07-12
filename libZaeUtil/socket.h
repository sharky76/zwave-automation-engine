
#include "event_dispatcher.h"
#include <poll.h>
#include <sys/uio.h>
#include "byte_buffer.h"

void  http_server_create(int socket, void (*http_event_handler)(event_pump_t*,int,void*));
int socket_create_server(int port, void(*on_connect)(event_pump_t*,int,void*), void* context);
int socket_recv(event_pump_t* pump, int fd, byte_buffer_t* buffer);
int socket_send(event_pump_t* pump, int fd, byte_buffer_t* buffer);
int socket_send_v(event_pump_t* pump, int fd, struct iovec* iov, int iovcnt, int len);