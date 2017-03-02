#include "vty.h"

void    socket_write_cb(vty_t* vty, const char* buf, size_t len);
char*   socket_read_cb(vty_t* vty);
void    socket_flush_cb(vty_t* vty);
void    socket_erase_cb(vty_t* vty);
void    socket_erase_line_cb(vty_t* vty);
void    socket_cursor_left_cb(vty_t* vty);
void    socket_cursor_right_cb(vty_t* vty);

