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

#define VERSION 23
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404
#define DENIED    401

DECLARE_LOGGER(HTTPServer)

char*       request_get_data(const char* request);
char*       request_create_command(const char* req_url);
int         header_find_value(const char* name, struct phr_header* headers, int header_size);

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
	static char buffer[BUFSIZE+1]; /* static so zero filled */

	const char *method, *path;
    int pret, minor_version;
    struct phr_header headers[100];
    size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    ssize_t rret;

    while(1) {
        /* read */
        rret = recv(client_socket, buffer + buflen, sizeof(buffer) - buflen, MSG_WAITALL);
        if (rret <= 0)
        {
            http_server_error_response(FORBIDDEN, client_socket);
            return NULL;
        }

        
        prevbuflen = buflen;
        buflen += rret;

        /* parse the request */
        num_headers = sizeof(headers) / sizeof(headers[0]);
        pret = phr_parse_request(buffer, buflen, &method, &method_len, &path, &path_len,
                                 &minor_version, headers, &num_headers, prevbuflen);
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

    LOG_DEBUG(HTTPServer, "Request %s", buffer);

    if(strncmp(method, "OPTIONS", method_len) == 0)
    {
        http_set_response(http_priv, HTTP_RESP_OK);
        http_set_content_type(http_priv, "application/json");
        http_set_header(http_priv, "Access-Control-Allow-Headers", "x-requested-with");
        http_server_write_response(client_socket, http_priv);
        return NULL;
    }

    int content_len_index = header_find_value("Content-Length", headers, num_headers);
    char* content = NULL;
    if(-1 != content_len_index)
    {
        char content_len_buf[10] = {0};
        strncpy(content_len_buf, headers[content_len_index].value, headers[content_len_index].value_len);
        int content_len = atoi(content_len_buf);

        if(content_len > 0)
        {
            content = malloc(content_len);
            strncpy(content, request_get_data(buffer), content_len);
            http_priv->post_data = content;
            http_priv->post_data_size = content_len;
        }
    }

    if(user_manager_get_count() > 0)
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
    }

    // Browsers really like to get favicon.ico - tell them to fuck off
    if(strstr(method, "favicon.ico") != 0)
    {
        http_server_error_response(NOTFOUND, client_socket);
        return NULL;
    }

    char* request_body = calloc(method_len + path_len + 2, sizeof(char));
    strncat(request_body, method, method_len);
    strncat(request_body, " ", 1);
    strncat(request_body, path, path_len);

    char* request_command = request_create_command(request_body);
    free(request_body);

    return request_command;
}

void  http_server_write_response(int client_socket, http_vty_priv_t* http_priv)
{
    static char buffer[BUFSIZE+1];
    //LOG_DEBUG(HTTPServer, "Sending %d bytes: %s", http_priv->response_size, http_priv->response);

    http_set_header(http_priv, "Access-Control-Allow-Origin", "*");
    http_set_header(http_priv, "Access-Control-Allow-Methods", "GET,POST,PUT");
    switch(http_priv->resp_code)
    {
    case HTTP_RESP_OK:
        sprintf(buffer, "HTTP/1.1 200 OK\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\n%sContent-Type: %s\n", 
                http_priv->response_size, 
                (http_priv->headers_size == 0)? "" : http_priv->headers, http_priv->content_type); /* Header + a blank line */
        sprintf(buffer + strlen(buffer), "Cache-Control: %s, max-age=%d", (http_priv->can_cache)? "public" : "no-cache", http_priv->cache_age);
        break;
    case HTTP_RESP_USER_ERR:
        sprintf(buffer, "HTTP/1.1 400 BAD REQUEST\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", http_priv->response_size, http_priv->content_type); /* Header + a blank line */
        break;
    case HTTP_RESP_SERVER_ERR:
        sprintf(buffer, "HTTP/1.1 500 SERVER ERROR\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", http_priv->response_size, http_priv->content_type); /* Header + a blank line */
        break;
    default:
        sprintf(buffer, "HTTP/1.1 400 BAD REQUEST\nServer: zae/0.1\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", http_priv->response_size, http_priv->content_type); /* Header + a blank line */
        break;
    }


    LOG_DEBUG(HTTPServer, "Response %s\n\n%s", buffer, http_priv->response);
    write(client_socket, buffer, strlen(buffer));

    if(http_priv->response_size > 0)
    {
        write(client_socket, "\n\n", 2);
        write(client_socket, http_priv->response, http_priv->response_size);
    }
}

void  http_set_response(http_vty_priv_t* http_priv, int http_resp)
{
    http_priv->resp_code = http_resp;
}

void  http_set_content_type(http_vty_priv_t* http_priv, const char* content_type)
{
    http_priv->content_type = strdup(content_type);
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
    http_priv->headers[http_priv->headers_size] = 0;
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

int       header_find_value(const char* name, struct phr_header* headers, int header_size)
{
    for (int i = 0; i < header_size; i++)
    {
        if(strncmp(headers[i].name, name, headers[i].name_len) == 0)
        {
            return i;
        }
    }

    return -1;
}

