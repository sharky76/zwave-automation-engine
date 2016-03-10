#include "vty.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "http_server.h"

#define BUFSIZE 8096

void    file_write_cb(vty_t* vty, const char* format, va_list args);
char*   file_read_cb(vty_t* vty);
void    std_write_cb(vty_t* vty, const char* format, va_list args);
char*   std_read_cb(vty_t* vty);
void    socket_write_cb(vty_t* vty, const char* format, va_list args);
char*   socket_read_cb(vty_t* vty);

char*   http_read_cb(vty_t* vty);
void    http_write_cb(vty_t* vty, const char* format, va_list args);
void    http_flush_cb(vty_t* vty);

vty_t*  vty_create(vty_type type, vty_data_t* data)
{
    vty_t* vty = (vty_t*)calloc(1, sizeof(vty_t));
    vty->type = type;
    vty->data = data;
    vty->echo = true;

    switch(vty->type)
    {
    case VTY_FILE:
        //rl_instream = vty->data->desc.file;
        //rl_outstream = vty->data->desc.file;
        vty->write_cb = file_write_cb;
        vty->read_cb = file_read_cb;
        break;
    case VTY_STD:
        //rl_callback_handler_install (vty->prompt, cb_linehandler);
        rl_instream = vty->data->desc.io_pair[IN];
        rl_outstream = vty->data->desc.io_pair[OUT];
        vty->write_cb = std_write_cb;
        vty->read_cb = std_read_cb;
        break;
    case VTY_HTTP:
        vty->write_cb = http_write_cb;
        vty->read_cb = http_read_cb;
        vty->flush_cb = http_flush_cb;
        vty->buffer = calloc(BUFSIZE, sizeof(char));

    /*case VTY_SOCKET:
        rl_instream = vty->data->desc.socket;
        rl_outstream = vty->data->desc.socket;
        vty->write_cb = std_write_cb;
        vty->read_cb = std_read_cb;*/
    }

    return vty;
}

void    vty_set_echo(vty_t* vty, bool is_echo)
{
    vty->echo = is_echo;
}

void    vty_free(vty_t* vty)
{
    if(NULL != vty->flush_cb)
    {
        vty->flush_cb(vty);
    }

    if(vty->type == VTY_FILE)
    {
        fclose(vty->data->desc.file);
    }

    free(vty->prompt);
    free(vty->buffer);
    free(vty);
}

void    vty_set_prompt(vty_t* vty, const char* format, ...)
{
    free(vty->prompt);
    va_list args;
    va_start(args, format);

    char prompt[256] = {0};
    vsprintf(prompt, format, args);
    vty->prompt = strdup(prompt);
}

void    vty_write(vty_t* vty, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vty->write_cb(vty, format, args);
}

void    vty_error(vty_t* vty, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char* error_format_buf = calloc(strlen(format) + 4, sizeof(char));
    strcat(error_format_buf, "%% ");
    strcat(error_format_buf, format);
    vty->write_cb(vty, error_format_buf, args);
    free(error_format_buf);
}

char*   vty_read(vty_t* vty)
{
    char* str = vty->read_cb(vty);
    if(NULL != str)
    {
        if('\0' != *str && *str != '\n' && vty->type == VTY_STD)
        {
            add_history(str);
        }
    }

    return str;
}

void    file_write_cb(vty_t* vty, const char* format, va_list args)
{
    vfprintf(vty->data->desc.file, format, args);
}

char*   file_read_cb(vty_t* vty)
{
    char* buffer = calloc(2048, sizeof(char));
    char* res = fgets(buffer, 2048, vty->data->desc.file);

    if(NULL == res)
    {
        free(buffer);
    }

    return res;
}

void    std_write_cb(vty_t* vty, const char* format, va_list args)
{
    char buf[BUFSIZE+1] = {0};
    vsnprintf(buf, BUFSIZE, format, args);
    //vfprintf(rl_outstream, format, args);
    write(fileno(rl_outstream), buf, sizeof(buf));
}

char*   std_read_cb(vty_t* vty)
{
    /*fd_set fds;
    keep_running = true;
    
    rl_set_prompt(vty->prompt);
    while(keep_running)
    {
        FD_ZERO (&fds);
        FD_SET(fileno(rl_instream), &fds);    
    
        int r = select (fileno(rl_instream)+1, &fds, NULL, NULL, NULL);
        if (r < 0)
        {
            rl_callback_handler_remove ();
            return NULL;
        }
        else if (FD_ISSET (fileno (rl_instream), &fds))
        {
            rl_callback_read_char();
        }
    }

    //rl_callback_handler_remove ();*/

    if(!vty->echo)
    {
        fprintf(rl_outstream, "%s", vty->prompt);
        fflush(rl_outstream);

        char* line = calloc(128, sizeof(char));
        fgets(line, 127, rl_instream);
        fprintf(rl_outstream, "\n");
        fflush(rl_outstream);

        return line;
    }
    else
    {
        return readline(vty->prompt);
    }
    //return rl_line_buffer;
}

char*   http_read_cb(vty_t* vty)
{
    int socket = vty->data->desc.socket;
    return strdup(http_server_read_request(socket));
}

void    http_write_cb(vty_t* vty, const char* format, va_list args)
{
    char buf[BUFSIZE+1] = {0};
    int size = vsnprintf(buf, BUFSIZE, format, args);

    if(vty->buf_size + size < BUFSIZE)
    {
        vty->buf_size += size;
        strncat(vty->buffer, buf, size);
    }
    else
    {
        http_flush_cb(vty);
    }
}

void    http_flush_cb(vty_t* vty)
{
    if(vty->buf_size > 0)
    {
        int socket = vty->data->desc.socket;
        http_server_write_response(socket, vty->buffer, vty->buf_size);

        vty->buf_size = 0;
        *vty->buffer = 0;
    }
}

void    socket_write_cb(vty_t* vty, const char* format, va_list args)
{

}

char*   socket_read_cb(vty_t* vty)
{

}

