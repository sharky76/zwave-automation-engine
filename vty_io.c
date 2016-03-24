#include "vty_io.h"
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
        vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    case VTY_STD:
        //rl_callback_handler_install (vty->prompt, cb_linehandler);
        rl_instream = vty->data->desc.io_pair[IN];
        rl_outstream = vty->data->desc.io_pair[OUT];
        vty->write_cb = std_write_cb;
        vty->read_cb = std_read_cb;
        vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    case VTY_HTTP:
        vty->write_cb = http_write_cb;
        vty->read_cb = http_read_cb;
        vty->flush_cb = http_flush_cb;
        vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    }
}


void    file_write_cb(vty_t* vty, const char* format, va_list args)
{
    vfprintf(vty->data->desc.file, format, args);
}

char*   file_read_cb(vty_t* vty)
{
    if(vty->multi_line)
    {
        char ch = 0;
        vty->buf_size = 0;
        *vty->buffer = 0;
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
    else
    {
        char* buffer = calloc(2048, sizeof(char));
        char* res = fgets(buffer, 2048, vty->data->desc.file);
    
        if(NULL == res)
        {
            free(buffer);
        }
    
        return res;
    }
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
    else if(vty->multi_line)
    {
        int ch = 0;
        vty->buf_size = 0;
        *vty->buffer = 0;
        while(true)
        {
            // Read multiple lines of input into buffer until \n.\n is found
            ch = fgetc(rl_instream);
            if(ch == EOF)
            {
                return NULL;
            }
            
            if(ch == 127)
            {
                if(vty->buf_size > 0)
                {
                    vty->buffer[--vty->buf_size] = 0;
                    fprintf(rl_outstream, "\b \b");
                    fflush(rl_outstream);
                }
                continue;
            }
            if(ch == vty->multiline_stop_char)
            {
                break;
            }
            fprintf(rl_outstream, "%c", ch);
            fflush(rl_outstream);
            if(vty->buf_size + 1 < BUFSIZE)
            {
                vty->buffer[vty->buf_size++] = ch;
            }
        }
        return vty->buffer;
    }
    else
    {
        char* line = readline(vty->prompt);
        if(NULL != line && vty->echo)
        {
            if('\0' != *line && *line != '\n' && vty->type == VTY_STD)
            {
                add_history(line);
            }
        }
        return line;
    }
    //return rl_line_buffer;
}

char*   http_read_cb(vty_t* vty)
{
    int socket = vty->data->desc.socket;
    return http_server_read_request(socket);
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
    //if(vty->buf_size > 0)
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

