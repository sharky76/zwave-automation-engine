#include "vty.h"

bool    socket_write_cb(vty_t* vty, const char* buf, size_t len);
int     socket_read_cb(vty_t* vty, char* str);
bool    socket_flush_cb(vty_t* vty);
void    socket_erase_cb(vty_t* vty);
void    socket_erase_line_cb(vty_t* vty);
void    socket_cursor_left_cb(vty_t* vty);
void    socket_cursor_right_cb(vty_t* vty);
void    socket_close_cb(vty_io_data_t* vty_data);

