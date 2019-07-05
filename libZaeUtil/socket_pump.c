#include "socket_pump.h"
#include "logger.h"
#include <stdint.h>

USING_LOGGER(EventPump);

typedef struct on_event_holder_t
{
    void (*on_read_event)(event_pump_t*, int, void* context);
    void (*on_write_event)(event_pump_t*, int, void* context);
    void* context;
} on_event_holder_t;

typedef struct socket_pump_data_t
{
    struct pollfd fds[MAX_POLLFDS+1];
    on_event_holder_t event_callbacks[MAX_POLLFDS+1];
    uint16_t free_index;
} socket_pump_data;

void socket_pump_init(event_pump_t* pump)
{
    LOG_ADVANCED(EventPump, "Initializing Socket Pump...");

    pump->name = strdup("SOCKET_PUMP");
    pump->poll = &socket_pump_poll;
    pump->register_handler = &socket_pump_add_sock;
    pump->unregister_handler = &socket_pump_remove_sock;
    pump->start = NULL;
    pump->stop = NULL;
    pump->priv = (void*)malloc(sizeof(socket_pump_data));
    socket_pump_data* data = (socket_pump_data*)pump->priv;

    memset(data->fds, 0, sizeof(data->fds));
    memset(data->event_callbacks, 0, sizeof(data->event_callbacks));
    data->free_index = 0;
}

void socket_pump_add_sock(event_pump_t* pump, va_list args)
{
    int sock_fd = va_arg(args, int);
    void(*on_read_event)(event_pump_t*, int,void*);
    on_read_event = va_arg(args, void(*)(event_pump_t*,int,void*));
    void(*on_write_event)(event_pump_t*, int,void*);
    on_write_event = va_arg(args, void(*)(event_pump_t*,int,void*));

    void* context = va_arg(args, void*);

    socket_pump_data* data = (socket_pump_data*)pump->priv;
    if(data->free_index == MAX_POLLFDS) return;

    data->fds[data->free_index].fd = sock_fd;
    data->fds[data->free_index].events = POLLIN|POLLOUT|POLLPRI;
    data->event_callbacks[data->free_index].on_read_event = on_read_event;
    data->event_callbacks[data->free_index].on_write_event = on_write_event;
    data->event_callbacks[data->free_index].context = context;
    data->free_index++;

    LOG_ADVANCED(EventPump, "New socket handler %d added", sock_fd);
}

void socket_pump_remove_sock(event_pump_t* pump, va_list args)
{
    int sock_fd = va_arg(args, int);

    socket_pump_data* data = (socket_pump_data*)pump->priv;
    for(int i = 0; i < data->free_index; i++)
    {
        if(data->fds[i].fd == sock_fd)
        {
            data->fds[i] = data->fds[data->free_index-1];
            data->event_callbacks[i] = data->event_callbacks[data->free_index-1];
            memset(&data->fds[data->free_index-1], 0, sizeof(struct pollfd));
            data->free_index--;
            LOG_ADVANCED(EventPump, "Socket handler %d removed", sock_fd);
            return;
        }
    }

    LOG_ERROR(EventPump, "Unable to remove socket handler %d: not found", sock_fd);
}

void socket_pump_poll(event_pump_t* pump, struct timespec* ts)
{
    socket_pump_data* data = (socket_pump_data*)pump->priv;

    int ret = poll(data->fds, data->free_index, 1);

    if(ret > 0)
    {
        for(int i = 0; i < data->free_index; i++)
        {
            if(data->fds[i].revents) 
            {
                on_event_holder_t* h = &data->event_callbacks[i];
                if((data->fds[i].revents & POLLIN) && h->on_read_event)
                {
                    h->on_read_event(pump, data->fds[i].fd, h->context);
                }

                if((data->fds[i].revents & POLLOUT) && h->on_write_event)
                {
                    h->on_write_event(pump, data->fds[i].fd, h->context);
                }
            }
        }
    }
}