#include "socket_io.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "stack.h"
#include "variant.h"
#include <logger.h>
#include <arpa/telnet.h>
#include <stdlib.h>
#include <arpa/inet.h>

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

void telnet_naws_negotiation_start(vty_t* vty)
{
    int socket = vty->data->desc.socket;
    char width[2];
    char height[2];
    int n = recv(socket, &width, 2, 0);
    n = recv(socket, &height, 2, 0);

    memcpy(&vty->term_width, width, 2);
    vty->term_width = ntohs(vty->term_width);
    memcpy(&vty->term_height, height, 2);
    vty->term_height = ntohs(vty->term_height);
}

void telnet_subnegotiation_start(vty_t* vty)
{
    int socket = vty->data->desc.socket;
    char ch;
    int n = recv(socket, &ch, 1, 0);
    switch(ch)
    {
    case TELOPT_NAWS:
        telnet_naws_negotiation_start(vty);
        break;
    default:
        break;
    }
}

bool    socket_write_cb(vty_t* vty, const char* buf, size_t len)
{
    //printf("Sending: %s\n", buf);
    //fflush(stdin);
    return send(vty->data->desc.socket, buf, len, 0) != -1;
}

int     socket_read_cb(vty_t* vty, char** str)
{
    int socket = vty->data->desc.socket;
    *str = calloc(1, sizeof(char));
    int n = recv(socket, *str, 1, 0);
    if(vty->iac_started || *str[0] == IAC)
    {
        //printf("NAWS received");
        vty->iac_started = true;
        
        switch(*str[0])
        {
        case SB:
            telnet_subnegotiation_start(vty);
            break;
        case SE:
            vty->iac_started = false;
            n = recv(socket, *str, 1, 0);
            break;
        default:
            break;
        }
    }
    //printf("RECV: %d %c (0x%x)\n", n, **str, **str);
    return n;

}

bool    socket_flush_cb(vty_t* vty)
{
    memset(vty->buffer, 0, vty->buf_size);
    vty->buf_size = 0;
    return true;
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

