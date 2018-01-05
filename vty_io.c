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
#include <termios.h>
#include <sys/ioctl.h>

bool    file_write_cb(vty_t* vty, const char* buf, size_t len);
int     file_read_cb(vty_t* vty, char** str);
bool    std_write_cb(vty_t* vty, const char* buf, size_t len);
int     std_read_cb(vty_t* vty, char** str);
void    std_erase_cb(vty_t* vty);
void    std_erase_line_cb(vty_t* vty);
void    std_cursor_left_cb(vty_t* vty);
void    std_cursor_right_cb(vty_t* vty);

int     http_read_cb(vty_t* vty, char** str);
bool    http_write_cb(vty_t* vty, const char* buf, size_t len);
bool    http_flush_cb(vty_t* vty);
void    http_free_priv_cb(vty_t* vty);

vty_t*  vty_io_create(vty_type type, vty_io_data_t* data)
{
    vty_t* new_vty = vty_create(type, data);
    new_vty->stored_vty = new_vty;
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
        vty->write_cb = std_write_cb;
        vty->read_cb = std_read_cb;
        vty->erase_char_cb = std_erase_cb;
        vty->erase_line_cb = std_erase_line_cb;
        vty->cursor_left_cb = std_cursor_left_cb;
        vty->cursor_right_cb = std_cursor_right_cb;

        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        vty->term_height = w.ws_row;
        vty->term_width = w.ws_col;
        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    case VTY_HTTP:
        vty->write_cb = http_write_cb;
        vty->read_cb = http_read_cb;
        vty->flush_cb = http_flush_cb;
        vty->free_priv_cb = http_free_priv_cb;
        http_vty_priv_t* http_priv = calloc(1, sizeof(http_vty_priv_t));
        http_priv->resp_code = HTTP_RESP_OK;
        vty->priv = http_priv;

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
        unsigned char cmd_do_naws[] = { IAC, DO, TELOPT_NAWS };
        
        //unsigned char cmd_will_cr[] = {IAC, WILL, TELOPT_NAOCRD};

        socket_write_cb(vty, cmd_will_echo, 3);
        socket_write_cb(vty, cmd_will_sga, 3);
        socket_write_cb(vty, cmd_dont_linemode, 3);
        socket_write_cb(vty, cmd_do_naws, 3);

        //socket_write_cb(vty, cmd_will_cr, 3);
        
        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    }
}


bool    file_write_cb(vty_t* vty, const char* buf, size_t len)
{
    return fwrite(buf, len, 1, vty->data->desc.file) == 1;
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

bool    std_write_cb(vty_t* vty, const char* buf, size_t len)
{
    bool retVal = false;
    retVal = write(fileno(vty->data->desc.io_pair[OUT]), buf, len) != -1;
    if(retVal)
    {
        fflush(vty->data->desc.io_pair[OUT]);
    }

    return retVal;
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
    http_vty_priv_t* http_priv = (http_vty_priv_t*)vty->priv;
    char* resp = http_server_read_request(socket, http_priv);

    if(NULL != resp)
    {
        *str = resp;
        return strlen(resp);
    }
    else
    {
        return 0;
    }
}

bool    http_write_cb(vty_t* vty, const char* buf, size_t len)
{
    http_vty_priv_t* http_priv = (http_vty_priv_t*)vty->priv;
    
    http_priv->response = realloc(http_priv->response, http_priv->response_size + len + 1);
    http_priv->response[http_priv->response_size] = 0;
    strncat(http_priv->response, buf, len);
    http_priv->response_size += len;
}

bool    http_flush_cb(vty_t* vty)
{
    bool retVal = false;
    http_vty_priv_t* http_priv = (http_vty_priv_t*)vty->priv;
    int socket = vty->data->desc.socket;

    //if(http_priv->response_size > 0)
    {
        retVal = http_server_write_response(socket, http_priv);
    
        if(http_priv->response_size > 0)
        {
            *http_priv->response = 0;
            http_priv->response_size = 0;
        }
    }
    vty->buf_size = 0;
    *vty->buffer = 0;

    return retVal;
}

void    http_free_priv_cb(vty_t* vty)
{
    http_vty_priv_t* http_priv = (http_vty_priv_t*)vty->priv;

    free(http_priv->request);
    free(http_priv->response);
    free(http_priv->content_type);
    free(http_priv->post_data);
    free(http_priv->headers);
    free(http_priv);
}
