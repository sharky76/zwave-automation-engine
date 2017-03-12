#include "vty_io.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "http_server.h"
#include "socket_io.h"
#include <arpa/telnet.h>

void    file_write_cb(vty_t* vty, const char* buf, size_t len);
int     file_read_cb(vty_t* vty, char** str);
void    std_write_cb(vty_t* vty, const char* buf, size_t len);
int     std_read_cb(vty_t* vty, char** str);
void    std_erase_cb(vty_t* vty);
void    std_erase_line_cb(vty_t* vty);
void    std_cursor_left_cb(vty_t* vty);
void    std_cursor_right_cb(vty_t* vty);

int     http_read_cb(vty_t* vty, char** str);
void    http_write_cb(vty_t* vty, const char* buf, size_t len);
void    http_flush_cb(vty_t* vty);

vty_t*  vty_io_create(vty_type type, vty_data_t* data)
{
    vty_t* new_vty = vty_create(type, data);
    vty_io_config(new_vty);
    return new_vty;
}

void    vty_io_config(vty_t* vty)
{
    switch(vty->type)
    {
    case VTY_FILE:
        //rl_instream = vty->data->desc.file;
        //rl_outstream = vty->data->desc.file;
        vty->write_cb = file_write_cb;
        vty->read_cb = file_read_cb;
        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    case VTY_STD:
        //rl_callback_handler_install (vty->prompt, cb_linehandler);
        rl_instream = vty->data->desc.io_pair[IN];
        rl_outstream = vty->data->desc.io_pair[OUT];
        vty->write_cb = std_write_cb;
        vty->read_cb = std_read_cb;
        vty->erase_char_cb = std_erase_cb;
        vty->erase_line_cb = std_erase_line_cb;
        vty->cursor_left_cb = std_cursor_left_cb;
        vty->cursor_right_cb = std_cursor_right_cb;

        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    case VTY_HTTP:
        vty->write_cb = http_write_cb;
        vty->read_cb = http_read_cb;
        vty->flush_cb = http_flush_cb;
        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    case VTY_SOCKET:
        vty->write_cb = socket_write_cb;
        vty->read_cb = socket_read_cb;
        vty->flush_cb = socket_flush_cb;
        vty->erase_char_cb = socket_erase_cb;
        vty->erase_line_cb = socket_erase_line_cb;
        vty->cursor_left_cb = socket_cursor_left_cb;
        vty->cursor_right_cb = socket_cursor_right_cb;

        //char iac_sga_buf[3] = {255, 251, 3};
        
        unsigned char cmd_will_echo[] = { IAC, WILL, TELOPT_ECHO };
        unsigned char cmd_will_sga[] = { IAC, WILL, TELOPT_SGA };
        unsigned char cmd_dont_linemode[] = { IAC, DONT, TELOPT_LINEMODE };
        //unsigned char cmd_will_cr[] = {IAC, WILL, TELOPT_NAOCRD};

        socket_write_cb(vty, cmd_will_echo, 3);
        socket_write_cb(vty, cmd_will_sga, 3);
        socket_write_cb(vty, cmd_dont_linemode, 3);
        //socket_write_cb(vty, cmd_will_cr, 3);
        
        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    }
}


void    file_write_cb(vty_t* vty, const char* buf, size_t len)
{
    fwrite(buf, len, 1, vty->data->desc.file);
}

int   file_read_cb(vty_t* vty, char** str)
{
    /*if(vty->multi_line)
    {
        char ch = 0;
        memset(vty->buffer, 0, vty->buf_size);
        vty->buf_size = 0;
        while(true)
        {
            // Read multiple lines of input into buffer until \n.\n is found
            ch = fgetc(vty->data->desc.file);
            if(ch == vty->multiline_stop_char)
            {
                break;
            }
            if(vty->buf_size + 1 < BUFSIZE)
            {
                vty->buffer[vty->buf_size++] = ch;
            }
        }
        return vty->buffer;
    }
    else*/
    {
        /*char* buffer = calloc(2048, sizeof(char));
        char* res = fgets(buffer, 2048, vty->data->desc.file);
    
        if(NULL == res)
        {
            free(buffer);
        }
    
        return res;*/
        char ch = 0;
        
        /*while((ch = fgetc(vty->data->desc.file)) != '\n')
        {
            // Read multiple lines of input into buffer until \n.\n is found
            if(vty->buf_size + 1 < BUFSIZE)
            {
                vty->buffer[vty->buf_size++] = ch;
            }
        }

        return vty->buffer;*/

        *str = malloc(sizeof(char));
        *str[0] = fgetc(vty->data->desc.file);

        if(**str == (char)EOF)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
}

void    std_write_cb(vty_t* vty, const char* buf, size_t len)
{
    write(fileno(vty->data->desc.io_pair[OUT]), buf, len);
    fflush(vty->data->desc.io_pair[OUT]);
}

int   std_read_cb(vty_t* vty, char** str)
{
    *str = calloc(1, sizeof(char));

    return read(fileno(vty->data->desc.io_pair[IN]), *str, 1);
}

void std_erase_cb(vty_t* vty)
{
    vty_write(vty, "\b \b");
}

void    std_erase_line_cb(vty_t* vty)
{
    vty_write(vty, "\33[2K\r");
}

void    std_cursor_left_cb(vty_t* vty)
{
    vty_write(vty, "\33[1D");
}

void    std_cursor_right_cb(vty_t* vty)
{
    vty_write(vty, "\33[1C");
}

int   http_read_cb(vty_t* vty, char** str)
{
    int socket = vty->data->desc.socket;
    char* resp = http_server_read_request(socket);
    *str = strdup(resp);
    return strlen(resp);
}

void    http_write_cb(vty_t* vty, const char* buf, size_t len)
{
    if(vty->buf_size + len < BUFSIZE)
    {
        vty->buf_size += len;
        strncat(vty->buffer, buf, len);
    }
    else
    {
        http_flush_cb(vty);
    }
}

void    http_flush_cb(vty_t* vty)
{
    //if(vty->buf_size > 0)
    {
        int socket = vty->data->desc.socket;
        http_server_write_response(socket, vty->buffer, vty->buf_size);

        vty->buf_size = 0;
        *vty->buffer = 0;
    }
}

