#include "vty.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define HISTORY_START   -1

vty_t*  vty_create(vty_type type, vty_data_t* data)
{
    vty_t* vty = (vty_t*)calloc(1, sizeof(vty_t));
    vty->type = type;
    vty->data = data;
    vty->echo = true;
    vty->multi_line = false;
    vty->error = false;
    vty->history = stack_create();
    vty->history_size = 0;
    vty->history_index = HISTORY_START;
    vty->buffer = calloc(BUFSIZE, sizeof(char));
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

    if(vty->type == VTY_SOCKET || vty->type == VTY_HTTP)
    {
        close(vty->data->desc.socket);
    }

    free(vty->prompt);
    free(vty->buffer);
    stack_free(vty->history);
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

void    vty_display_prompt(vty_t* vty)
{
    vty_write(vty, vty->prompt);
}

void    vty_set_banner(vty_t* vty, char* banner)
{
    vty->banner = banner;
}

void    vty_display_banner(vty_t* vty)
{
    if(vty->banner != NULL)
    {
        vty_write(vty, "%s\r\n", vty->banner);
    }
}

void    vty_write(vty_t* vty, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buf[BUFSIZE+1] = {0};
    int len = vsnprintf(buf, BUFSIZE, format, args);

    vty->write_cb(vty, buf, len);
}

void    vty_error(vty_t* vty, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char* error_format_buf = calloc(strlen(format) + 4, sizeof(char));
    strcat(error_format_buf, "%% ");
    strcat(error_format_buf, format);
    vty_write(vty, error_format_buf, args);
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

void    vty_flush(vty_t* vty)
{
    if(NULL != vty->flush_cb)
    {
        vty->flush_cb(vty);
    }
}

void    vty_set_error(vty_t* vty, bool is_error)
{
    vty->error = is_error;
}

bool    vty_is_error(vty_t* vty)
{
    return vty->error;
}

void    vty_add_history(vty_t* vty)
{
    variant_t* recent_entry = stack_peek_front(vty->history);
    if(vty->buf_size > 0 && 
       (NULL == recent_entry || strcmp(variant_get_string(recent_entry), vty->buffer))
      )
    {
        stack_push_front(vty->history, variant_create_string(strdup(vty->buffer)));
        if(vty->history->count > vty->history_size)
        {
            variant_t* victim = stack_pop_back(vty->history);
            variant_free(victim);
        }
    }

    vty->history_index = HISTORY_START;
}

const char*    vty_get_history(vty_t* vty, bool direction) // true - to newest, false - to oldest
{
    const char* hist_item = NULL;
    
    if(direction && vty->history_index > HISTORY_START)
    {
        if(vty->history_index == vty->history->count)
        {
            vty->history_index--;
        }
        vty->history_index--;
    }
    else if(!direction && vty->history_index < vty->history->count)
    {
        vty->history_index++;
    }

    if(vty->history_index > HISTORY_START && vty->history_index < vty->history->count)
    {
        hist_item = variant_get_string(stack_peek_at(vty->history, vty->history_index));
    }

    return hist_item;
}

void    vty_set_history_size(vty_t* vty, int size)
{
    vty->history_size = size;

    while(vty->history->count > size)
    {
        variant_t* victim = stack_pop_back(vty->history);
        variant_free(victim);
    }
}

void    vty_insert_char(vty_t* vty, char ch)
{
    if(vty->buf_size < BUFSIZE)
    {
        if(vty->cursor_pos < vty->buf_size)
        {
            // need to insert the char
            char newbuf[BUFSIZE] = {0};
            strncpy(newbuf, vty->buffer, vty->cursor_pos);
            strncat(newbuf, &ch, 1);
            strncat(newbuf, vty->buffer + vty->cursor_pos, vty->buf_size - vty->cursor_pos);

            //printf("Old Buffer: %s , New buffer: %s\n", vty->buffer, newbuf);

            vty->buf_size++;
            vty->cursor_pos++;
            int saved_cursor = vty->cursor_pos;
            vty_redisplay(vty, newbuf);

            while(vty->cursor_pos > saved_cursor)
            {
                vty_cursor_left(vty);
            }
        }
        else
        {
            //printf("Appending, New buffer: %s\n", vty->buffer);
            // need to append the char
            vty->buffer[vty->buf_size++] = ch;
            vty->cursor_pos = vty->buf_size;
            vty_write(vty, "%c", ch);
        }
    }
}

void    vty_append_char(vty_t* vty, char ch)
{
    while(vty->cursor_pos < vty->buf_size)
    {
        vty_cursor_right(vty);
    }

    vty_insert_char(vty, ch);
}

void    vty_append_string(vty_t* vty, const char* str)
{
    int len = strlen(str);

    if(vty->buf_size + len < BUFSIZE)
    {
        strncpy(vty->buffer + vty->buf_size, str, len);
        vty->buf_size += len;
        vty->cursor_pos = vty->buf_size;
        vty_write(vty, str);
    }
}

void    vty_erase_char(vty_t* vty)
{
    if(vty->buf_size > 0 && vty->cursor_pos > 0)
    {
        if(vty->cursor_pos < vty->buf_size)
        {
            char newbuf[BUFSIZE] = {0};
            strncpy(newbuf, vty->buffer, vty->cursor_pos - 1);
            strncat(newbuf, vty->buffer + vty->cursor_pos, vty->buf_size - vty->cursor_pos);

            vty->buf_size--;
            vty->cursor_pos--;
            int saved_cursor = vty->cursor_pos;
            vty_redisplay(vty, newbuf);

            while(vty->buf_size != 0 && vty->cursor_pos > saved_cursor)
            {
                vty_cursor_left(vty);
            }
        }
        else
        {
            vty->buffer[vty->buf_size] = 0;
            vty->buf_size--;
            vty->cursor_pos = vty->buf_size;

            if(NULL != vty->erase_char_cb)
            {
                vty->erase_char_cb(vty);
            }
        }

    }
}

void    vty_redisplay(vty_t* vty, const char* new_buffer)
{
    //if(NULL != new_buffer)
    {
        if(NULL != vty->erase_line_cb)
        {
            vty->erase_line_cb(vty);
            vty_display_prompt(vty);
        }
        else
        {
            while(vty->buf_size > 0)
            {
                vty_erase_char(vty);
            }
        }

        vty->cursor_pos = 0;
    
        if(NULL != new_buffer)
        {
            strncpy(vty->buffer, new_buffer, BUFSIZE-1);
            vty->buf_size = strlen(vty->buffer);
            vty->cursor_pos = vty->buf_size;
            vty_write(vty, vty->buffer);

        }
    }
}

void    vty_clear_buffer(vty_t* vty)
{
    memset(vty->buffer, 0, vty->buf_size);
    vty->buf_size = 0;
    vty->cursor_pos = 0;
}

void    vty_show_history(vty_t* vty)
{
    if(NULL != vty->show_history_cb)
    {
        vty->show_history_cb(vty);
    }
    else
    {
        stack_for_each_reverse(vty->history, history_variant)
        {
            vty_write(vty, "%s\r\n", variant_get_string(history_variant));
        }
    }
}

void    vty_cursor_left(vty_t* vty)
{
    if(vty->cursor_pos > 0)
    {
        if(NULL != vty->cursor_left_cb)
        {
            vty->cursor_left_cb(vty);
        }

        vty->cursor_pos--;
    }
}

void    vty_cursor_right(vty_t* vty)
{
    if(vty->cursor_pos < vty->buf_size)
    {
        if(NULL != vty->cursor_right_cb)
        {
            vty->cursor_right_cb(vty);
        }

        vty->cursor_pos++;
    }
}

