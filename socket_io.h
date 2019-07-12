#include "vty.h"

void    socket_configure_terminal(vty_t* vty);
bool    socket_write_cb(vty_t* vty);
int     socket_read_cb(vty_t* vty, char* str);
bool    socket_flush_cb(vty_t* vty);
void    socket_erase_cb(vty_t* vty);
void    socket_erase_line_cb(vty_t* vty);
void    socket_cursor_left_cb(vty_t* vty);
void    socket_cursor_right_cb(vty_t* vty);
void    socket_close_cb(vty_io_data_t* vty_data);
