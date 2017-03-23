#include <stdbool.h>

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

typedef struct http_vty_priv_t
{
    char* response;
    int   response_size;
    char* content_type;
    char* post_data;
    int   post_data_size;
    int   resp_code;
    bool  can_cache;
    int   cache_age;
} http_vty_priv_t;

int   http_server_get_socket(int port);
char* http_server_read_request(int client_socket, http_vty_priv_t* http_priv);
void  http_server_write_response(int client_socket, http_vty_priv_t* http_priv);
void  http_set_response(http_vty_priv_t* http_priv, int http_resp);
void  http_set_content_type(http_vty_priv_t* http_priv, const char* content_type);
void  http_set_cache_control(http_vty_priv_t* http_priv, bool is_set, int max_age);



