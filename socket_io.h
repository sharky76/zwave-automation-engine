#include "vty.h"

void    socket_write_cb(vty_t* vty, const char* format, va_list args);
char*   socket_read_cb(vty_t* vty);
void    socket_flush_cb(vty_t* vty);
void    socket_erase_cb(vty_t* vty);
void    socket_erase_line_cb(vty_t* vty);
