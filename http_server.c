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
#include <base64.h>
#include "user_manager.h"
#include "picohttpparser.h"
#include "vty.h"
#include "cli_commands.h"
#include "cli_rest.h"
#include "event.h"

#define VERSION 23
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404
#define DENIED    401

DECLARE_LOGGER(HTTPServer)

char*       request_get_data(const char* request);
char*       request_create_command(const char* req_url);
int         header_find_value(const char* name, struct phr_header* headers, int header_size);

void  http_request_handle_event(event_t* event, void* context);

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
            event_register_handler(HTTPServer, "HTTPRequest", &http_request_handle_event,  NULL);
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
    case DENIED:
        write(socket_fd, "HTTP/1.1 401 Access denied\nWWW-Authenticate: Basic realm=\"ZWAVE Automation Server\"\nContent-Length: 0\n", 101);
        break;
	}	
}

char* http_server_read_request(int client_socket, http_vty_priv_t* http_priv)
{
    int j, file_fd;
	long i, ret, len;
	char * fstr;

    http_priv->request = calloc(1, sizeof(http_request_t));

    int pret, minor_version;
    size_t buflen = 0, prevbuflen = 0;
    ssize_t rret;

    while(1) {
        /* read */
        rret = recv(client_socket, http_priv->request->buffer + buflen, sizeof(http_priv->request->buffer) - buflen, 0);
        if (rret <= 0)
        {
            http_server_error_response(FORBIDDEN, client_socket);
            return NULL;
        }

        prevbuflen = buflen;
        buflen += rret;

        /* parse the request */
        http_priv->request->num_headers = sizeof(http_priv->request->headers) / sizeof(http_priv->request->headers[0]);
        pret = phr_parse_request(http_priv->request->buffer, buflen, &http_priv->request->method, &http_priv->request->method_len, &http_priv->request->path, &http_priv->request->path_len,
                                 &minor_version, http_priv->request->headers, &http_priv->request->num_headers, prevbuflen);
        if (pret > 0)
        {
                break; /* successfully parsed the request */
        }
        else if(pret == -1)
        {
            http_server_error_response(FORBIDDEN, client_socket);
            return NULL;
        }
    };

    LOG_DEBUG(HTTPServer, "Request %s", http_priv->request->buffer);

    if(strncmp(http_priv->request->method, "OPTIONS", http_priv->request->method_len) == 0)
    {
        http_set_response(http_priv, HTTP_RESP_OK);
        http_set_content_type(http_priv, "application/json");
        http_set_header(http_priv, "Access-Control-Allow-Headers", "x-requested-with,content-type");
        http_server_write_response(client_socket, http_priv);
        return NULL;
    }

    // Handle POST request
    if(strncmp(http_priv->request->method, "POST", http_priv->request->method_len) == 0)
    {
        char* content = NULL;
        char* content_len_buf;
        if(http_request_find_header_value(http_priv, "Content-Length", &content_len_buf))
        {
            int content_len = atoi(content_len_buf);
    
            if(content_len > 0)
            {
                content = calloc(1, content_len+1);
                //http_priv->post_data = calloc(1, content_len+1);
                strncpy(content, request_get_data(http_priv->request->buffer), content_len);
    
                http_priv->post_data = content;
                http_priv->post_data_size = content_len;
            }
            free(content_len_buf);
        }
    }

    /*if(user_manager_get_count() > 0)
    {
        char* auth_start = strstr(buffer, "Authorization: Basic");
        if(NULL == auth_start)
        {
            LOG_ERROR(HTTPServer, "Access denied");
            http_server_error_response(DENIED, client_socket);
            return NULL;
        }
        else
        {
            char user_pass[256] = {0};
            sscanf(auth_start, "Authorization: Basic %s", user_pass);
    
            char user_pass_decoded[256] = {0};
            Base64decode(user_pass_decoded, user_pass);
    
            char* user_tok = strtok(user_pass_decoded, ":");
            char* pass_tok = strtok(NULL, ":");
            if(!user_manager_authenticate(user_tok, pass_tok))
            {
                LOG_ERROR(HTTPServer, "Access denied");
                http_server_error_response(DENIED, client_socket);
                return NULL;
            }
            else
            {
                LOG_INFO(HTTPServer, "Authentication success for %s\n", user_tok);
            }
        }
    }*/

    // Browsers really like to get favicon.ico - tell them to fuck off
    if(strstr(http_priv->request->method, "favicon.ico") != 0)
    {
        http_server_error_response(NOTFOUND, client_socket);
        return NULL;
    }

    char* request_body = calloc(http_priv->request->method_len + http_priv->request->path_len + 2, sizeof(char));
    strncat(request_body, http_priv->request->method, http_priv->request->method_len);
    strncat(request_body, " ", 1);
    strncat(request_body, http_priv->request->path, http_priv->request->path_len);

    char* request_command = request_create_command(request_body);
    free(request_body);

    return request_command;
}

bool  http_server_write_response(int client_socket, http_vty_priv_t* http_priv)
{
    char buffer[HTTP_REQUEST_BUFSIZE+1] = {0};
    //LOG_DEBUG(HTTPServer, "Sending %d bytes: %s", http_priv->response_size, http_priv->response);

    switch(http_priv->resp_code)
    {
    case HTTP_RESP_OK:
        http_set_header(http_priv, "Access-Control-Allow-Origin", "*");
        http_set_header(http_priv, "Access-Control-Allow-Methods", "GET,POST,PUT");
        sprintf(buffer, "HTTP/1.1 200 OK\nServer: zae/0.1\n%s", 
                (http_priv->headers_size == 0)? "" : http_priv->headers); /* Header + a blank line */

        if(http_priv->can_cache)
        {
            sprintf(buffer + strlen(buffer), "Cache-Control: public, max-age=%d\n", http_priv->cache_age);
        }
        else
        {
            sprintf(buffer + strlen(buffer), "Cache-Control: no-cache\n");
        }

        if(NULL != http_priv->content_type)
        {
            sprintf(buffer + strlen(buffer), "Content-Type: %s\n", http_priv->content_type);
        }

        if(http_priv->response_size > 0)
        {
            sprintf(buffer + strlen(buffer), "Content-Length: %ld\n", http_priv->response_size);
        }

        sprintf(buffer + strlen(buffer), "\n");

        break;
    case HTTP_RESP_USER_ERR:
        sprintf(buffer, "HTTP/1.1 400 BAD REQUEST\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", http_priv->response_size, http_priv->content_type); /* Header + a blank line */
        break;
    case HTTP_RESP_SERVER_ERR:
        sprintf(buffer, "HTTP/1.1 500 SERVER ERROR\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", http_priv->response_size, http_priv->content_type); /* Header + a blank line */
        break;
    case HTTP_RESP_NONE:
        break;
    default:
        sprintf(buffer, "HTTP/1.1 400 BAD REQUEST\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", http_priv->response_size, http_priv->content_type); /* Header + a blank line */
        break;
    }


    bool retVal = false;
    LOG_DEBUG(HTTPServer, "Response %s\n\n%s", buffer, http_priv->response);
    retVal = write(client_socket, buffer, strlen(buffer)) != -1;

    if(retVal && http_priv->response_size > 0)
    {
        //retVal = write(client_socket, "\n\n", 2) != -1;

        //if(retVal)
        {
            retVal = write(client_socket, http_priv->response, http_priv->response_size) != -1;
        }
    }

    return retVal;
}

void  http_set_response(http_vty_priv_t* http_priv, int http_resp)
{
    http_priv->resp_code = http_resp;
}

void  http_set_content_type(http_vty_priv_t* http_priv, const char* content_type)
{
    if(NULL != http_priv->content_type)
    {
        free(http_priv->content_type);
        http_priv->content_type = NULL;
    }

    if(NULL != content_type)
    {
        http_priv->content_type = strdup(content_type);
    }
}

void  http_set_cache_control(http_vty_priv_t* http_priv, bool is_set, int max_age)
{
    http_priv->can_cache = is_set;
    http_priv->cache_age = max_age;
}

void  http_set_header(http_vty_priv_t* http_priv, const char* name, const char* value)
{
    int header_size = strlen(name) + strlen(value) + 4;
    http_priv->headers = realloc(http_priv->headers, http_priv->headers_size + header_size);

    sprintf(http_priv->headers + http_priv->headers_size, "%s: %s\n", name, value);
    http_priv->headers_size += header_size;
    http_priv->headers[http_priv->headers_size-1] = 0;
    http_priv->headers_size--;
}

char*       request_create_command(const char* req_url)
{
    char* command = strdup(req_url);

    for(int i = 0; i < strlen(req_url); i++) 
    { 
		if(req_url[i] == '/') 
        { 
			command[i] = ' ';
		}
	}

    // expression spaces will be encoded as %20 - we need to revert them back to ' '
    // we will try to do in-place conversion...
    int body_len = strlen(command);
    int string_index = 0;
    for(int i = 0; i < body_len; i++,string_index++) 
    {
        if(i+2 < body_len)
        {
            if(command[i] == '%' && command[i+1] == '2' && command[i+2] == '0')
            {
                // forward i to past-%20 place
                i += 2;
                command[i] = ' ';
            }
        }

        if(i != string_index)
        {
            command[string_index] = command[i];
        }
    }

    command[string_index] = '\0';

    return command;
}

char*       request_get_data(const char* request)
{
    char* start_data = strstr(request, "\r\n\r\n");
    if(NULL == start_data)
    {
        return NULL;
    }

    return start_data + 4;
}

int   http_request_find_header_value_index(http_vty_priv_t* http_priv, const char* name)
{
    for (int i = 0; i < http_priv->request->num_headers; i++)
    {
        if(strncasecmp(http_priv->request->headers[i].name, name, http_priv->request->headers[i].name_len) == 0)
        {
            return i;
        }
    }

    return -1;
}

bool http_request_find_header_value_by_index(http_vty_priv_t* http_priv, int index, char** value)
{
    if(index >= 0)
    {
        *value = calloc(http_priv->request->headers[index].value_len+1, sizeof(char));
        strncpy(*value, http_priv->request->headers[index].value, http_priv->request->headers[index].value_len);
        return true;
    }

    return false;
}

bool  http_request_find_header_value(http_vty_priv_t* http_priv, const char* name, char** value)
{
    return http_request_find_header_value_by_index(http_priv,
                                                   http_request_find_header_value_index(http_priv, name),
                                                   value);
}

void  http_request_handle_event(event_t* event, void* context)
{
    //variant_stack_t* http_socket_list = (variant_stack_t*)context;
    vty_t* http_vty = VARIANT_GET_PTR(vty_t, event->data);

    char* str = vty_read(http_vty);

    if(NULL != str)
    {
        cli_command_exec_custom_node(rest_root_node, http_vty, str);
    }

    vty_free(http_vty);
    LOG_ADVANCED(HTTPServer, "HTTP request completed");
}
