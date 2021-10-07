#include "socket.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "socket_pump.h"

USING_LOGGER(General)

int socket_create_server(int port, void(*on_connect)(event_pump_t*,int,void*), void* context)
{
    int server_sock;
	socklen_t length;
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

    if((server_sock = socket(AF_INET, SOCK_STREAM,0)) <0)
    {
		LOG_ERROR(General, "socket %s", strerror(errno));
    }
    else
    {
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);
        int on = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
        
        int flags = fcntl(server_sock, F_GETFL);
        fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);

        if(bind(server_sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
        {
            LOG_ERROR(General, "bind: %s", strerror(errno));
        }
        else if(listen(server_sock, 1) <0)
        {
            LOG_ERROR(General, "listen %s", strerror(errno));
        }
        else
        {
            LOG_ADVANCED(General, "Server initialized on port %d", port);
            event_dispatcher_register_handler(event_dispatcher_get_pump("SOCKET_PUMP"), server_sock, on_connect, NULL, context);
            return server_sock;
        }
    }

    return -1;
}

/**
 * Return values:
 * 
 * 0    need to try again
 * -1   error, close socket
 * > 0  data received 
 */
int socket_recv(event_pump_t* pump, int fd, byte_buffer_t* buffer)
{
    int spare_len = byte_buffer_spare_len(buffer);
    int ret = recv(fd, byte_buffer_get_write_ptr(buffer), spare_len, MSG_DONTWAIT);

    if(ret <= 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }
        else
        {
            //event_dispatcher_unregister_handler(pump, fd, &socket_stop_callback, &fd)
            return -1;
        }
    }

    byte_buffer_adjust_write_pos(buffer, ret);

    return ret;
}

int socket_send(event_pump_t* pump, int fd, byte_buffer_t* buffer)
{
    //if(0 == byte_buffer_read_len(buffer)) return 0;

    // Add POLLOUT event only during send...
    socket_pump_set_event_flags(pump, fd, POLLOUT, true);

    int ret = send(fd, byte_buffer_get_read_ptr(buffer), byte_buffer_read_len(buffer), MSG_DONTWAIT);

    if(ret < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        { 
            return 0;
        }
        else 
        {
            socket_pump_set_event_flags(pump, fd, POLLOUT, false);
            return -1;
        }
        
    }

    byte_buffer_adjust_read_pos(buffer, ret);
    byte_buffer_pack(buffer);

    if(byte_buffer_read_len(buffer) == 0)
    {
        socket_pump_set_event_flags(pump, fd, POLLOUT, false);
        return ret;
    }

    return 0;
}

int socket_send_v(event_pump_t* pump, int fd, struct iovec* iov, int iovcnt, int len)
{
    // Add POLLOUT event only during send...
    socket_pump_set_event_flags(pump, fd, POLLOUT, true);

    int ret = writev(fd, iov, iovcnt);

    if(ret < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }
        else
        {
            socket_pump_set_event_flags(pump, fd, POLLOUT, false);
            return -1;
        }
    }

    if(ret == len)
    {
        socket_pump_set_event_flags(pump, fd, POLLOUT, false);
    }

    return ret;
}
