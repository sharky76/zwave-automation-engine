#include "socket_io.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "stack.h"
#include "variant.h"
#include <logger.h>
#include <arpa/telnet.h>
#include <stdlib.h>

DECLARE_LOGGER(SocketIO)

#define KEY_TAB       0x09
#define KEY_BACKSPACE 0x7f
#define KEY_ESC       0x1b
#define KEY_BRACKET   0x5b
#define KEY_UP_ARROW  0x41
#define KEY_DOWN_ARROW 0x42
#define KEY_RIGHT_ARROW 0x43
#define KEY_LEFT_ARROW 0x44
#define KEY_NEWLINE     0xa
#define KEY_RETURN      0xd

void    socket_write_cb(vty_t* vty, const char* buf, size_t len)
{
    //printf("Sending: %s\n", buf);
    //fflush(stdin);
    send(vty->data->desc.socket, buf, len, 0);
}

int     socket_read_cb(vty_t* vty, char** str)
{
    int socket = vty->data->desc.socket;
    *str = calloc(1, sizeof(char));
    int n = recv(socket, *str, 1, 0);
    //printf("RECV: %d %c (0x%x)\n", n, **str, **str);
    return n;

}

void    socket_flush_cb(vty_t* vty)
{
    memset(vty->buffer, 0, vty->buf_size);
    vty->buf_size = 0;
}

void    socket_erase_cb(vty_t* vty)
{
    vty_write(vty, "\b \b");
}

void    socket_erase_line_cb(vty_t* vty)
{
    vty_write(vty, "\33[2K\r");
}

void    socket_cursor_left_cb(vty_t* vty)
{
    vty_write(vty, "\33[1D");
}

void    socket_cursor_right_cb(vty_t* vty)
{
    vty_write(vty, "\33[1C");
}

