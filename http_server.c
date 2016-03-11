#include "http_server.h"
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
#include <logger.h>

#define VERSION 23
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404

DECLARE_LOGGER(HTTPServer)

int  http_server_get_socket(int port)
{
    int server_sock;
	socklen_t length;
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

    if((server_sock = socket(AF_INET, SOCK_STREAM,0)) <0)
    {
		LOG_ERROR(HTTPServer, "socket %s", strerror(errno));
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
            LOG_ERROR(HTTPServer, "bind: %s", strerror(errno));
        }
        else if(listen(server_sock, 1) <0)
        {
            LOG_ERROR(HTTPServer, "listen %s", strerror(errno));
        }
        else
        {
            return server_sock;
        }
    }

    return -1;
}

void http_server_error_response(int type, int socket_fd)
{
	switch (type) 
    {
	case FORBIDDEN: 
		write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n",271);
		break;
	case NOTFOUND: 
		write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
		break;
	}	
}

char* http_server_read_request(int client_socket)
{
    int j, file_fd, buflen;
	long i, ret, len;
	char * fstr;
	static char buffer[BUFSIZE+1]; /* static so zero filled */

	ret = read(client_socket, buffer, BUFSIZE); 	/* read Web request in one go */
	if(ret == 0 || ret == -1) 
    {	/* read failure stop now */
		http_server_error_response(FORBIDDEN, client_socket);
        return NULL;
	}

	if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
    {
		buffer[ret]=0;		/* terminate the buffer */
    }
	else
    {
        buffer[0]=0;
    }

    for(i=0; i<ret; i++)	/* remove CF and LF characters */
    {
		if(buffer[i] == '\r' || buffer[i] == '\n')
        {
            buffer[i]='*';
        }
    }

	LOG_DEBUG(HTTPServer, "Request %s", buffer);

	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ) 
    {
		http_server_error_response(FORBIDDEN, client_socket);
        return NULL;
	}

    for(i = 4; i < BUFSIZE; i++) 
    { /* null terminate after the second space to ignore extra stuff */
		if(buffer[i] == ' ') 
        { /* string is "GET URL " +lots of other stuff */
			buffer[i] = 0;
			break;
		}
	}

    char* request_body = &buffer[5];

	for(i = 0; i < strlen(request_body); i++) 
    { 
		if(request_body[i] == '/') 
        { 
			request_body[i] = ' ';
		}
	}

    // expression spaces will be encoded as %20 - we need to revert them back to ' '
    // we will try to do in-place conversion...
    int body_len = strlen(request_body);
    int string_index = 0;
    for(i = 0; i < body_len; i++,string_index++) 
    {
        if(i+2 < body_len)
        {
            if(request_body[i] == '%' && request_body[i+1] == '2' && request_body[i+2] == '0')
            {
                // forward i to past-%20 place
                i += 2;
                request_body[i] = ' ';
            }
        }

        if(i != string_index)
        {
            request_body[string_index] = request_body[i];
        }
    }
    // Now, add the last '\0' character
    request_body[string_index] = '\0';

    // Browsers really like to get favicon.ico - tell them to fuck off
    if(strcmp(request_body, "favicon.ico") == 0)
    {
        http_server_error_response(NOTFOUND, client_socket);
        return NULL;
    }

    // ok, we have request
    return request_body;
}

void  http_server_write_response(int client_socket, const char* data, int size)
{
    static char buffer[BUFSIZE+1];
    LOG_DEBUG(HTTPServer, "Sending %d bytes", size);
    sprintf(buffer, "HTTP/1.1 200 OK\nServer: zae/0.%d\nContent-Length: %ld\nConnection: close\nContent-Type: text/plain\n\n", 1, size); /* Header + a blank line */
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, data, size);
}

