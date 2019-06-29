#include "socket_pump.h"

typedef struct socket_pump_data_t
{
    struct pollfd fds[MAX_POLLFDS+1];
    uint16_t free_index;
    hash_table_t* event_callback_table;
} socket_pump_data;


void socket_pump_init(event_pump_t* pump)
{
    pump->name = strdup("SOCKET_PUMP");
    pump->poll = &socket_pump_poll;
    pump->register_handler = &socket_pump_add_sock;
    pump->unregister_handler = &socket_pump_remove_sock;
    pump->start = NULL;
    pump->stop = NULL;
    pump->priv = (socket_pump_data*)malloc(sizeof(socket_pump_data));
    socket_pump_data* data = (socket_pump_data*)pump->priv;

    memset(data->fds, 0, sizeof(data->fds));
    data->free_index = 0;
    data->event_callback_table = variant_hash_init();
}

typedef struct on_event_holder_t
{
    void (*on_event)(struct pollfd*);
} on_event_holder;

void socket_pump_add_sock(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int sock_fd = va_arg(args, int);
    int flags = va_arg(args, int);
    void(*on_event)(struct pollfd*);
    on_event = va_arg(args, void(*)(struct pollfd*));
    va_end(args);

    socket_pump_data* data = (socket_pump_data*)pump->priv;
    if(data->free_index == MAX_POLLFDS) return;

    data->fds[data->free_index].fd = sock_fd;
    data->fds[data->free_index].events = flags;
    data->free_index++;

    on_event_holder* event_handler = (on_event_holder*)malloc(sizeof(on_event_holder));
    event_handler->on_event = on_event;
    variant_hash_insert(data->event_callback_table, sock_fd, variant_create_ptr(DT_PTR, event_handler, variant_delete_default));
}

void socket_pump_remove_sock(event_pump_t* pump, ...)
{
    va_list args;
    va_start(args, pump);
    int sock_fd = va_arg(args, int);
    va_end(args);

    socket_pump_data* data = (socket_pump_data*)pump->priv;
    for(int i = 0; i < data->free_index; i++)
    {
        if(data->fds[i].fd == sock_fd)
        {
            data->fds[i] = data->fds[data->free_index-1];
            memset(&data->fds[data->free_index-1], 0, sizeof(struct pollfd));
            data->free_index--;
            variant_hash_remove(data->event_callback_table, sock_fd);
            return;
        }
    }
}

void socket_pump_poll(event_pump_t* pump, struct timespec* ts)
{
    socket_pump_data* data = (socket_pump_data*)pump->priv;

    int ret = poll(data->fds, data->free_index, 0);

    if(ret > 0)
    {
        for(int i = 0; i < data->free_index; i++)
        {
            if((data->fds[i].revents & POLLIN|POLLPRI && data->fds[i].events & POLLIN|POLLPRI) ||
                (data->fds[i].revents & POLLOUT && data->fds[i].events & POLLOUT))
            {
                variant_t* on_event_variant = variant_hash_get(data->event_callback_table, data->fds[i].fd);
                on_event_holder* h = (on_event_holder*)variant_get_ptr(on_event_variant);
                h->on_event(&data->fds[i]);
            }
        }
    }
}