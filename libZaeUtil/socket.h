
void  http_server_register_with_event_loop(int socket, void (*http_event_handler)(int,void*), void* arg);
int socket_create_server(int port);

