#include "vty.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

void    file_write_cb(vty_t* vty, const char* format, va_list args);
char*   file_read_cb(vty_t* vty);
void    std_write_cb(vty_t* vty, const char* format, va_list args);
char*   std_read_cb(vty_t* vty);
void    socket_write_cb(vty_t* vty, const char* format, va_list args);
char*   socket_read_cb(vty_t* vty);

vty_t*  vty_create(vty_type type, vty_data_t* data)
{
    vty_t* vty = (vty_t*)calloc(1, sizeof(vty_t));
    vty->type = type;
    vty->data = data;

    switch(vty->type)
    {
    case VTY_FILE:
        //rl_instream = vty->data->desc.file;
        //rl_outstream = vty->data->desc.file;
        vty->write_cb = file_write_cb;
        vty->read_cb = file_read_cb;
        break;
    case VTY_STD:
        vty->write_cb = std_write_cb;
        vty->read_cb = std_read_cb;
        break;
    }

    return vty;
}

void    vty_free(vty_t* vty)
{
    if(vty->type == VTY_FILE)
    {
        fclose(vty->data->desc.file);
    }

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
    char* error_format_buf = calloc(strlen(format) + 3, sizeof(char));
    strcat(error_format_buf, "%% ");
    strcat(error_format_buf, format);
    vty->write_cb(vty, error_format_buf, args);
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
    vfprintf(stdout, format, args);
}

char*   std_read_cb(vty_t* vty)
{
    return readline(vty->prompt);
}


void    socket_write_cb(vty_t* vty, const char* format, va_list args)
{

}

char*   socket_read_cb(vty_t* vty)
{

}

