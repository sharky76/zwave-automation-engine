#include "socket_pump.h"
#include "logger.h"
#include <stdint.h>
#include <pthread.h>

USING_LOGGER(EventPump);

typedef struct on_event_holder_t
{
    void (*on_read_event)(event_pump_t*, int, void* context);
    void (*on_write_event)(event_pump_t*, int, void* context);
    void* context;
    bool  remove_pending_flag;
    void (*on_unsubscribe)(void*);
    void* unsubscribe_arg;
} on_event_holder_t;

typedef struct socket_pump_data_t
{
    struct pollfd fds[MAX_POLLFDS+1];
    on_event_holder_t event_callbacks[MAX_POLLFDS+1];
    uint16_t free_index;
    pthread_mutex_t pump_lock;
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

    pthread_mutex_init(&data->pump_lock, NULL);
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
    pthread_mutex_lock(&data->pump_lock);

    if(data->free_index == MAX_POLLFDS) 
    {
        LOG_ERROR(EventPump, "Socket subscription table full");
        pthread_mutex_unlock(&data->pump_lock);

        return;
    }

    data->fds[data->free_index].fd = sock_fd;
    data->event_callbacks[data->free_index].on_read_event = on_read_event;
    data->event_callbacks[data->free_index].on_write_event = on_write_event;
    data->fds[data->free_index].events = POLLIN|POLLPRI;
    data->event_callbacks[data->free_index].context = context;
    data->event_callbacks[data->free_index].remove_pending_flag = false;
    data->event_callbacks[data->free_index].on_unsubscribe = NULL;
    data->event_callbacks[data->free_index].unsubscribe_arg = NULL;

    data->free_index++;
    pthread_mutex_unlock(&data->pump_lock);

    LOG_ADVANCED(EventPump, "New socket handler %d added", sock_fd);
}

void socket_pump_remove_sock(event_pump_t* pump, va_list args)
{
    int sock_fd = va_arg(args, int);
    void(*on_unsubscribe)(void*);
    on_unsubscribe = va_arg(args, void(*)(void*));
    void* unsubscribe_arg = va_arg(args, void*);

    socket_pump_data* data = (socket_pump_data*)pump->priv;

    pthread_mutex_lock(&data->pump_lock);

    for(int i = 0; i < data->free_index; i++)
    {
        if(data->fds[i].fd == sock_fd)
        {
            data->event_callbacks[i].remove_pending_flag = true;
            data->event_callbacks[i].on_unsubscribe = on_unsubscribe;
            data->event_callbacks[i].unsubscribe_arg = unsubscribe_arg;
            pthread_mutex_unlock(&data->pump_lock);

            return;
        }
    }

    pthread_mutex_unlock(&data->pump_lock);

    LOG_ERROR(EventPump, "Unable to remove socket handler %d: not found", sock_fd);
}

void socket_purge_sockets(event_pump_t* pump, int index)
{
    socket_pump_data* data = (socket_pump_data*)pump->priv;
    if(data->event_callbacks[index].remove_pending_flag)
    {
        if(data->event_callbacks[index].on_unsubscribe)
        {
            LOG_DEBUG(EventPump, "Removing socket, running unsubscriber callback: %d", data->fds[index].fd);
            data->event_callbacks[index].on_unsubscribe(data->event_callbacks[index].unsubscribe_arg);
        }

        LOG_ADVANCED(EventPump, "Socket handler %d removed", data->fds[index].fd);
        data->fds[index] = data->fds[data->free_index-1];
        data->event_callbacks[index] = data->event_callbacks[data->free_index-1];
        memset(&data->fds[data->free_index-1], 0, sizeof(struct pollfd));
        memset(&data->event_callbacks[data->free_index-1], 0, sizeof(struct on_event_holder_t));
        data->free_index--;
        LOG_DEBUG(EventPump, "Active sockets now: %d", data->free_index);
    }
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
                if((data->fds[i].revents & (POLLIN|POLLERR|POLLHUP)) && h->on_read_event)
                {
                    h->on_read_event(pump, data->fds[i].fd, h->context);
                }

                if((data->fds[i].revents & POLLOUT) && h->on_write_event)
                {
                    h->on_write_event(pump, data->fds[i].fd, h->context);
                }
            }
            
            if(data->event_callbacks[i].remove_pending_flag /*&& !(data->fds[i].events & POLLOUT)*/)
            {
                socket_purge_sockets(pump, i);
            }
        }
    }
}

void socket_pump_set_event_flags(event_pump_t* pump, int fd, int flags, bool is_set)
{
    socket_pump_data* data = (socket_pump_data*)pump->priv;

    for(int i = 0; i < data->free_index; i++)
    {
        if(data->fds[i].fd == fd)
        {
            if(is_set) 
            {
                data->fds[i].events |= flags;
            }
            else
            {
                data->fds[i].events &= ~flags;
            }
            return;
        }
    }

    return;
}
