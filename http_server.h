#include <stdbool.h>
#include "picohttpparser.h"
#include "vty.h"
#include "byte_buffer.h"

/*
Simple HTTP server implementation. 
Server accepts GET requests where URL path is cli commands separated by "/" 
 
Example: http://x.x.x.x:NN/show/running-config 
         http://x.x.x.x:NN/scene/Test   <- enter scene state
         http://x.x.x.x:NN/condition/true <- condition for scene Test
         http://x.x.x.x:NN/show/sensor/node-id/5/instance-id/0/command-id/128   <- show sensor data
*/

#define HTTP_RESP_OK         200
#define HTTP_RESP_USER_ERR   400
#define HTTP_RESP_SERVER_ERR 500
#define HTTP_RESP_NONE       900


#define CONTENT_TYPE_JSON   "application/json"
#define CONTENT_TYPE_TEXT   "text/plain"

typedef enum RequestType
{
    GET,
    POST,
    PUT,
    DELETE,
    UNKNOWN
} RequestType;

#define HTTP_REQUEST_BUFSIZE 8096
#define HTTP_RESPONSE_HEADERS_SIZE 1024

typedef struct http_request_t
{
    byte_buffer_t* buffer;
    const char *method;
    const char *path;
    struct phr_header headers[100];
    size_t method_len;
    size_t path_len;
    size_t num_headers;
} http_request_t;

typedef struct http_response_t
{
    byte_buffer_t* buffer;
    byte_buffer_t* response_header;
} http_response_t;

typedef struct http_vty_priv_t
{
    http_request_t* request;
    //byte_buffer_t* response;
    char* content_type;
    char* post_data;
    int   post_data_size;
    char* headers;
    int   headers_size;
    int   resp_code;
    bool  can_cache;
    int   cache_age;
    int   content_length;
    byte_buffer_t* response_header;
} http_vty_priv_t;

void   http_server_create(int port, void (*on_read)(event_pump_t*,int, void*));
void http_request_handle_connect_event(event_pump_t* pump, int fd, void* context);
void*  http_server_create_context(void (*http_event_read_handler)(event_pump_t* pump,int, void*));
int http_server_read_request(http_vty_priv_t* http_priv, byte_buffer_t* buffer);

void   http_server_prepare_response_headers(http_vty_priv_t* http_priv, int content_length);

void  http_set_response(http_vty_priv_t* http_priv, int http_resp);
void  http_set_content_type(http_vty_priv_t* http_priv, const char* content_type);
void  http_set_cache_control(http_vty_priv_t* http_priv, bool is_set, int max_age);
void  http_set_header(http_vty_priv_t* http_priv, const char* name, const char* value);
int   http_request_find_header_value_index(http_vty_priv_t* http_priv, const char* name);
bool  http_request_find_header_value_by_index(http_vty_priv_t* http_priv, int index, char** value);
bool  http_request_find_header_value(http_vty_priv_t* http_priv, const char* name, char** value);

char* http_response_get_post_data(vty_t* vty);

