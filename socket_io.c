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
#include <unistd.h>
#include <errno.h>
#include "socket.h"

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

void socket_configure_terminal(vty_t* vty)
{
    unsigned char cmd_will_echo[] = { IAC, WILL, TELOPT_ECHO };
    unsigned char cmd_will_sga[] = { IAC, WILL, TELOPT_SGA };
    unsigned char cmd_dont_linemode[] = { IAC, DONT, TELOPT_LINEMODE };
    unsigned char cmd_do_naws[] = { IAC, DO, TELOPT_NAWS };
    
    //unsigned char cmd_will_cr[] = {IAC, WILL, TELOPT_NAOCRD};

    byte_buffer_append(vty->write_buffer, cmd_will_echo, 3);
    byte_buffer_append(vty->write_buffer, cmd_will_sga, 3);
    byte_buffer_append(vty->write_buffer, cmd_dont_linemode, 3);
    byte_buffer_append(vty->write_buffer, cmd_do_naws, 3);

    vty_flush(vty);
}

bool telnet_naws_negotiation_start(vty_t* vty)
{
    if(byte_buffer_read_len(vty->read_buffer) < 5)
    {
        return false;
    }

    // skip TELOPT_NAWS
    byte_buffer_adjust_read_pos(vty->read_buffer, 1);
    
    char width[2];
    char height[2];

    memcpy(&vty->term_width, byte_buffer_get_read_ptr(vty->read_buffer), 2);
    byte_buffer_adjust_read_pos(vty->read_buffer, 2);
    vty->term_width = ntohs(vty->term_width);
    memcpy(&vty->term_height, byte_buffer_get_read_ptr(vty->read_buffer), 2);
    byte_buffer_adjust_read_pos(vty->read_buffer, 2);
    vty->term_height = ntohs(vty->term_height);
    byte_buffer_pack(vty->read_buffer);

    return true;
}

bool telnet_subnegotiation_start(vty_t* vty)
{
    int ret;

    if(byte_buffer_read_len(vty->read_buffer) < 1)
    {
        return false;
    }

    char ch = *byte_buffer_get_read_ptr(vty->read_buffer);
    switch(ch)
    {
    case TELOPT_NAWS:
        return telnet_naws_negotiation_start(vty);
    default:
        byte_buffer_adjust_read_pos(vty->read_buffer, 1);
        return false;
    }
}

bool    socket_write_cb(vty_t* vty)
{
    int ret = socket_send(vty_get_pump(vty), vty->data->desc.socket, vty->write_buffer);
    if(-1 == ret)
    {
        event_dispatcher_unregister_handler(vty_get_pump(vty), vty->data->desc.socket, &vty_free, (void*)vty);
    }

    return ret != -1;
}

int     socket_read_cb(vty_t* vty)
{
    int ret = socket_recv(vty_get_pump(vty), vty->data->desc.socket, vty->read_buffer);
    if(ret < 1)
    {
        if(-1 == ret)
        {
            event_dispatcher_unregister_handler(vty_get_pump(vty), vty->data->desc.socket, &vty_free, (void*)vty);
        }
        return 0;
    }

    char ch = *byte_buffer_get_read_ptr(vty->read_buffer);
    
    if(vty->iac_started || ch == IAC)
    {
        //printf("NAWS received");
        if(!vty->iac_started && ch == IAC)
        {
            vty->iac_started = true;
            byte_buffer_adjust_read_pos(vty->read_buffer, 1);
        }
        
        while(byte_buffer_read_len(vty->read_buffer) > 0 && vty->iac_started)
        {
            ch = *byte_buffer_get_read_ptr(vty->read_buffer);

            switch(ch)
            {
                case SE:
                    byte_buffer_adjust_read_pos(vty->read_buffer, 1);
                    vty->iac_started = false;
                    break;
                default:
                    telnet_subnegotiation_start(vty);
                    break;
            }
        }
    }
    return byte_buffer_read_len(vty->read_buffer);

}

bool    socket_flush_cb(vty_t* vty)
{
   if(byte_buffer_read_len(vty->write_buffer) > 0)
   {
       socket_write_cb(vty);
       return true;
   }

   return false;
}

void    socket_erase_cb(vty_t* vty)
{
    vty_write(vty, "\x8");
}

void    socket_delete_cb(vty_t* vty)
{
    vty_write(vty, "\x7f");
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

void    socket_close_cb(vty_io_data_t* vty_data)
{
    close(vty_data->desc.socket);
}
