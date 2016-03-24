#include "vty.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

vty_t*  vty_create(vty_type type, vty_data_t* data)
{
    vty_t* vty = (vty_t*)calloc(1, sizeof(vty_t));
    vty->type = type;
    vty->data = data;
    vty->echo = true;
    vty->multi_line = false;

    return vty;
}

void    vty_set_echo(vty_t* vty, bool is_echo)
{
    vty->echo = is_echo;
}

void    vty_set_multiline(vty_t* vty, bool is_multiline, char stop_char)
{
    vty->multi_line = is_multiline;

    if(is_multiline)
    {
        vty->multiline_stop_char = stop_char;
    }
    else
    {
        vty->multiline_stop_char = 0;
    }
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

void    vty_set_banner(vty_t* vty, char* banner)
{
    vty->banner = banner;
}

void    vty_display_banner(vty_t* vty)
{
    if(vty->banner != NULL)
    {
        vty_write(vty, "%s\n", vty->banner);
    }
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
    if(NULL != str && vty->echo)
    {
        /*if('\0' != *str && *str != '\n' && vty->type == VTY_STD)
        {
            add_history(str);
        }*/
    }

    return str;
}
