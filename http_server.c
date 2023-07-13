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
#include "vty_io.h"
#include "socket.h"

#define VERSION 23
#define ERROR      42
#define LOG        44
#define OK        200
#define FORBIDDEN 403
#define NOTFOUND  404
#define DENIED    401
#define OPTIONS   402

DECLARE_LOGGER(HTTPServer)

char*       request_get_data(const char* request);
int       request_create_command(const char* req_url, byte_buffer_t* buffer);
int         header_find_value(const char* name, struct phr_header* headers, int header_size);

typedef struct http_event_context_t
{
    void (*private_event_handler)(event_pump_t* pump,int, void*);
} http_event_context_t;

void   http_server_create(int port, void (*on_read)(event_pump_t*,int, void*))
{
    socket_create_server(port, &http_request_handle_connect_event, http_server_create_context(on_read));
}

void*  http_server_create_context(void (*http_event_read_handler)(event_pump_t* pump,int, void*))
{
    http_event_context_t* new_context = malloc(sizeof(http_event_context_t)); 
    new_context->private_event_handler = http_event_read_handler;
    return (void*)new_context;
}

void http_request_handle_connect_event(event_pump_t* pump, int fd, void* context)
{
    http_event_context_t* context_data = (http_event_context_t*)context;
    struct sockaddr remote_addr;
    socklen_t addr_len;
    memset(&remote_addr, 0, sizeof(struct sockaddr));
    memset(&addr_len, 0, sizeof(socklen_t));
    int session_sock = accept(fd, &remote_addr, &addr_len);
    
    struct sockaddr_in* addr_in = (struct sockaddr_in*)&remote_addr;
    struct in_addr ip_addr = addr_in->sin_addr;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ip_addr, ip_str, INET_ADDRSTRLEN );
    LOG_INFO(HTTPServer, "HTTP connection from %s", ip_str);

    vty_io_data_t* vty_data = malloc(sizeof(vty_io_data_t));


    vty_data->desc.socket = session_sock;

    vty_t* vty_sock = vty_io_create(VTY_HTTP, vty_data);
    vty_set_pump(vty_sock, pump);

    vty_set_echo(vty_sock, false);

    
    event_dispatcher_register_handler(pump, session_sock, context_data->private_event_handler, &vty_nonblock_write_event, (void*)vty_sock);
}

int http_server_read_request(http_vty_priv_t* http_priv, byte_buffer_t* buffer)
{
    int j, file_fd;
	long i, ret, len;
	char * fstr;

    http_set_response(http_priv, HTTP_RESP_OK);

    int pret, minor_version;
    size_t buflen = 0;
    ssize_t rret;

    buflen = byte_buffer_read_len(http_priv->request->buffer);

    LOG_DEBUG(HTTPServer, "Request length: %d", buflen);

    /* parse the request */
    http_priv->request->num_headers = sizeof(http_priv->request->headers) / sizeof(http_priv->request->headers[0]);
    pret = phr_parse_request(byte_buffer_get_read_ptr(http_priv->request->buffer), buflen, &http_priv->request->method, &http_priv->request->method_len, &http_priv->request->path, &http_priv->request->path_len,
                                &minor_version, http_priv->request->headers, &http_priv->request->num_headers, 0);
    if (pret > 0)
    {
        // The request has been parsed, now make sure we got the entire body...
        char* content_len_buf;
        if(http_request_find_header_value(http_priv, "Content-Length", &content_len_buf))
        {
            http_priv->content_length = atoi(content_len_buf);
            free(content_len_buf);
            if(http_priv->content_length > 0)
            {
                if(buflen - pret < http_priv->content_length)
                {
                    return 0;
                }

                char* request = request_get_data(byte_buffer_get_read_ptr(http_priv->request->buffer));

                if(request == NULL || strlen(request) < http_priv->content_length)
                {
                    // Read some more data...
                    return 0;
                }
            }
        }
    }
    else if(pret == -1)
    {
        LOG_ERROR(HTTPServer, "Parse error");
        http_set_response(http_priv, FORBIDDEN);
        //http_server_prepare_response_headers(http_priv);
        return 0;
    }
    else if(pret < 0)
    {
        return 0;
    }

    LOG_DEBUG(HTTPServer, "Request %s", byte_buffer_get_read_ptr(http_priv->request->buffer));

    if(user_manager_get_count() > 0)
    {
        char* auth_start;
        if(!http_request_find_header_value(http_priv, "Authorization", &auth_start))
        {
            LOG_ERROR(HTTPServer, "Access denied for request: %s", byte_buffer_get_read_ptr(http_priv->request->buffer));
            http_set_response(http_priv, DENIED);
            //http_server_prepare_response_headers(http_priv);
            return 0;

        }
        else
        {
            char user_pass[256] = {0};
            sscanf(auth_start, "Basic %s", user_pass);
    
            char user_pass_decoded[256] = {0};
            Base64decode(user_pass_decoded, user_pass);
            free(auth_start);

            char* user_tok = strtok(user_pass_decoded, ":");
            char* pass_tok = strtok(NULL, ":");
            if(!user_manager_authenticate(user_tok, pass_tok))
            {
                LOG_ERROR(HTTPServer, "Access denied, invalid username/password");
                http_set_response(http_priv, DENIED);
                //http_server_prepare_response_headers(http_priv);
                return 0;
            }
            else
            {
                LOG_INFO(HTTPServer, "Authentication success for %s", user_tok);
            }
        }
    }

    if(strncmp(http_priv->request->method, "OPTIONS", http_priv->request->method_len) == 0)
    {
        LOG_DEBUG(HTTPServer, "Received OPTIONS request");
        http_set_response(http_priv, HTTP_RESP_OK);
        http_set_content_type(http_priv, "application/json");
        http_set_header(http_priv, "Access-Control-Allow-Headers", "x-requested-with,content-type");
        //http_server_prepare_response_headers(http_priv);
        return 0;
    }

    // Handle POST request
    if(strncmp(http_priv->request->method, "POST", http_priv->request->method_len) == 0)
    {
        char* content = NULL;
        if(http_priv->content_length > 0)
        {
            content = calloc(1, http_priv->content_length+1);
            strncpy(content, request_get_data(byte_buffer_get_read_ptr(http_priv->request->buffer)), http_priv->content_length);
            http_priv->post_data = content;
        }
    }

    // Browsers really like to get favicon.ico - tell them to fuck off
    if(strstr(http_priv->request->path, "favicon.ico") != 0)
    {
        http_set_response(http_priv, NOTFOUND);
        //http_server_prepare_response_headers(http_priv);
        return 0;
    }

    char* request_body = calloc(http_priv->request->method_len + http_priv->request->path_len + 2, sizeof(char));
    strncat(request_body, http_priv->request->method, http_priv->request->method_len);
    strcat(request_body, " ");
    strncat(request_body, http_priv->request->path, http_priv->request->path_len);

    int command_len = request_create_command(request_body, buffer);
    free(request_body);

    return command_len;
}

void  http_server_prepare_response_headers(http_vty_priv_t* http_priv, int content_length)
{
        switch(http_priv->resp_code)
        {
        case HTTP_RESP_OK:
            http_set_header(http_priv, "Access-Control-Allow-Origin", "*");
            http_set_header(http_priv, "Access-Control-Allow-Methods", "GET,POST,PUT");
            byte_buffer_adjust_write_pos(http_priv->response_header, 
                sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "HTTP/1.1 200 OK\r\nServer: zae/0.1\r\n%s", 
                    (http_priv->headers_size == 0)? "" : http_priv->headers)); /* Header + a blank line */
            if(http_priv->can_cache)
            {
                byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "Cache-Control: public, max-age=%d\r\n", http_priv->cache_age));
            }
            else
            {
                byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "Cache-Control: no-cache\r\n"));
            }

            if(NULL != http_priv->content_type)
            {
                byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "Content-Type: %s\r\n", http_priv->content_type));
            }

            if(content_length > 0)
            {
                byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "Content-Length: %ld\r\n", content_length));
            }

            byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "\r\n"));

            break;
        case HTTP_RESP_SERVER_ERR:
            byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "HTTP/1.1 500 SERVER ERROR\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", content_length, http_priv->content_type)); /* Header + a blank line */
            break;
        case HTTP_RESP_NONE:
            break;
        case FORBIDDEN: 
            byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n"));
            break;
        case NOTFOUND: 
            byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n"));
            break;
        case DENIED:
            byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "HTTP/1.1 401 Access denied\nWWW-Authenticate: Basic realm=\"ZWAVE Automation Server\"\nContent-Length: 0\n"));
            break;
        case HTTP_RESP_USER_ERR:
        default:
            byte_buffer_adjust_write_pos(http_priv->response_header, sprintf(byte_buffer_get_write_ptr(http_priv->response_header), "HTTP/1.1 400 BAD REQUEST\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", content_length, http_priv->content_type)); /* Header + a blank line */
            break;
        }


        LOG_DEBUG(HTTPServer, "Response header size %d: %s", byte_buffer_read_len(http_priv->response_header), byte_buffer_get_read_ptr(http_priv->response_header));
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
    int header_size = strlen(name) + strlen(value) + 5;
    http_priv->headers = realloc(http_priv->headers, http_priv->headers_size + header_size);

    sprintf(http_priv->headers + http_priv->headers_size, "%s: %s\r\n", name, value);
    http_priv->headers_size += header_size;
    http_priv->headers[http_priv->headers_size-1] = 0;
    http_priv->headers_size--;
}

int       request_create_command(const char* req_url, byte_buffer_t* buffer)
{
    //char* command = strdup(req_url);
    char command_buffer[2048] = {0};
    strcpy(command_buffer, req_url);

    for(int i = 0; i < strlen(req_url); i++) 
    { 
		if(req_url[i] == '/') 
        { 
			command_buffer[i] = ' ';
		}
	}

    // expression spaces will be encoded as %20 - we need to revert them back to ' '
    // we will try to do in-place conversion...
    int body_len = strlen(command_buffer);
    int string_index = 0;
    for(int i = 0; i < body_len; i++,string_index++) 
    {
        if(i+2 < body_len)
        {
            if(command_buffer[i] == '%' && command_buffer[i+1] == '2' && command_buffer[i+2] == '0')
            {
                // forward i to past-%20 place
                i += 2;
                command_buffer[i] = ' ';
            }
        }

        if(i != string_index)
        {
            command_buffer[string_index] = command_buffer[i];
        }
    }

    command_buffer[string_index] = '\0';
    byte_buffer_append(buffer, command_buffer, strlen(command_buffer));

    return byte_buffer_read_len(buffer);
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

char* http_response_get_post_data(vty_t* vty)
{
    return ((http_vty_priv_t*)vty->priv)->post_data;
}

