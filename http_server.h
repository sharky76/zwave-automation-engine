/*
Simple HTTP server implementation. 
Server accepts GET requests where URL path is cli commands separated by "/" 
 
Example: http://x.x.x.x:NN/show/running-config 
         http://x.x.x.x:NN/scene/Test   <- enter scene state
         http://x.x.x.x:NN/condition/true <- condition for scene Test
         http://x.x.x.x:NN/show/sensor/node-id/5/instance-id/0/command-id/128   <- show sensor data
*/


int   http_server_get_socket(int port);
char* http_server_read_request(int client_socket);
void  http_server_write_response(int client_socket, const char* data, int size);




