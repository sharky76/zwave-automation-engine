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

int socket_create_server_old(int port)
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
            return server_sock;
        }
    }

    return -1;
}
