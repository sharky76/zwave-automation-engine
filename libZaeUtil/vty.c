#include "vty.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "cli.h"
#include <arpa/telnet.h>
#include <stdio.h>

#define HISTORY_START   -1
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
#define KEY_CTRL_Z      0x1a
#define KEY_CTRL_C      0x3

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
    vty->completions = stack_create();
    return vty;
}

void    vty_set_completions(vty_t* vty)
{
    stack_free(vty->completions);
    vty->completions = cli_get_command_completions(vty, vty->buffer, vty->buf_size);
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

    free(vty->data);
    free(vty->prompt);
    free(vty->buffer);
    stack_free(vty->history);
    stack_free(vty->completions);
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
        vty_write(vty, "%s%s", vty->banner, VTY_NEWLINE(vty));
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

    char buf[BUFSIZE+1] = {0};
    int len = vsnprintf(buf, BUFSIZE, format, args);

    char* error_format_buf = calloc(len + 4, sizeof(char));
    strcat(error_format_buf, "%% ");
    strcat(error_format_buf, buf);
    vty_write(vty, error_format_buf);
    free(error_format_buf);
}

char*   vty_read(vty_t* vty)
{
    if(vty_is_command_received(vty))
    {
        vty_clear_buffer(vty);
    }

    char* ch;
    vty_set_command_received(vty, false);

    //while(true)
    {
        int n = vty->read_cb(vty, &ch);

        if(n > 1)
        {
            vty_append_string(vty, ch);
        }
        else if(n <= 0)
        {
            vty_set_error(vty, true);
            vty_clear_buffer(vty);
            //break;
        }
        else if(vty->type == VTY_FILE && ch[0] == (char)EOF)
        {
            vty_set_error(vty, true);
            vty_clear_buffer(vty);
            //break;
        }
        else if(vty->multi_line)
        {
            if(ch[0] == vty->multiline_stop_char)
            {
                vty_set_command_received(vty, true);
                vty->multi_line = false;
                //break;
            }
            else
            {
                if(*ch == KEY_RETURN)
                {
                    vty_append_string(vty, VTY_NEWLINE(vty));
                }
                else if(*ch == KEY_NEWLINE)
                {
                    //printf("Char received: %c (0x%x)\n", *ch, *ch);
                    vty_append_string(vty, "%s", "\r\n");
                    //printf("buffer: 0x%x 0x%x\n", vty->buffer[vty->buf_size-2], vty->buffer[vty->buf_size-1]);
                }
                else
                {
                    vty_insert_char(vty, *ch);
                }
            }
        }
        else if(ch[0] == KEY_NEWLINE || ch[0] == KEY_RETURN || ch[n-1] == '\n' )
        {
            //printf("Line received: %s, multiline: %d\n", vty->buffer, vty->multi_line);
            vty_add_history(vty);
            vty_set_command_received(vty, true);
            //break;
        }
        else if(ch[0] == IAC)
        {
            //printf("IAC start\n");
            vty->iac_started = true;
            vty->iac_count = 1;
        }
        else if(vty->iac_started)
        {
            if(vty->iac_count < 2)
            {
                //printf("IAC cont\n");

                vty->iac_count++;
            }
            else
            {
                //printf("IAC end\n");

                vty->iac_started = false;
            }
        }
        else if(ch[0] == KEY_CTRL_Z)
        {
            vty_clear_buffer(vty);
            vty_append_string(vty, "end");
            vty_set_command_received(vty, true);
            //break;
        }
        else if(ch[0] == KEY_CTRL_C)
        {
            vty_clear_buffer(vty);
            vty_set_command_received(vty, true);
            //break;
        }
        else if(ch[0] == '?')
        {
            cli_command_describe_norl(vty);
        }
        else if(ch[0] == KEY_TAB) // tab
        {
            if(!vty->command_completion_started)
            {
                vty_set_completions(vty);
            
                //printf("Show 1 completiing for %s\r\n", vty->buffer);

                if(NULL != vty->completions)
                {
                    if(vty->completions->count == 1)
                    {
                        variant_stack_t* cmd_stack = create_cmd_vec(vty->buffer);
                        variant_t* incomplete_cmd = stack_pop_back(cmd_stack);
                        variant_t* complete_cmd = stack_pop_front(vty->completions);
    
                        // Redisplay the completed command
                        for(int i = 0; i < strlen(variant_get_string(incomplete_cmd)); i++)
                        {
                            vty_erase_char(vty);
                        }

                        const char* complete_cmd_string = variant_get_string(complete_cmd);
                        vty_append_string(vty, complete_cmd_string);
                        
                        vty_append_char(vty, ' ');
                        variant_free(incomplete_cmd);
                        variant_free(complete_cmd);
                        stack_free(vty->completions);
                        vty->completions = NULL;
                        stack_free(cmd_stack);

                        
                    }
                    else
                    {
                        vty->command_completion_started = true;
                    }
                }
            }
            else
            {   
                vty->command_completion_started = false;
                int word_count = 0;
                vty_write(vty, VTY_NEWLINE(vty));

                stack_for_each(vty->completions, matching_command)
                {
                    vty_write(vty, "%-20s", variant_get_string(matching_command));
                    if(word_count++ >= 3)
                    {
                        vty_write(vty, VTY_NEWLINE(vty));
                        word_count = 0;
                    }
                }

                if(word_count > 0)
                {
                    vty_write(vty, VTY_NEWLINE(vty));
                }

                vty_redisplay(vty, vty->buffer);
            }
        }
        else if(ch[0] == KEY_BACKSPACE) // backspace
        {
            vty_erase_char(vty);
        }
        else if(ch[0] == KEY_ESC) // ESC char
        {
            vty->esc_sequence_started = true;
        }
        else if(vty->esc_sequence_started)
        {
            if(ch[0] == KEY_BRACKET) // bracket char 
            {

            }
            else if(ch[0] == KEY_UP_ARROW)
            {
                const char* hist = vty_get_history(vty, false);

                if(NULL != hist)
                {
                    vty_redisplay(vty, hist);
                }

                vty->esc_sequence_started = false;
            }
            else if(ch[0] == KEY_DOWN_ARROW)
            {
                const char* hist = vty_get_history(vty, true);
                if(NULL == hist)
                {
                    vty_clear_buffer(vty);
                }
                vty_redisplay(vty, hist);
                vty->esc_sequence_started = false;

            }
            else if(ch[0] == KEY_LEFT_ARROW)
            {
                vty_cursor_left(vty);
                vty->esc_sequence_started = false;

            }
            else if(ch[0] == KEY_RIGHT_ARROW)
            {
                vty_cursor_right(vty);
                vty->esc_sequence_started = false;

                //break;
            }
        }
        else if(n == 1)
        {
            vty_insert_char(vty, *ch);
        }
        else
        {
            vty_append_string(vty, ch);
        }

        free(ch);
    }

    //free(ch);
    return vty->buffer;
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
    if(vty->buf_size < BUFSIZE && ch != 0x0)
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

            if(vty->echo)
            {
                vty_write(vty, "%c", ch);
            }
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

void    vty_append_string(vty_t* vty, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buf[BUFSIZE+1] = {0};
    int len = vsnprintf(buf, BUFSIZE, format, args);

    if(vty->buf_size + len < BUFSIZE)
    {
        strncpy(vty->buffer + vty->buf_size, buf, len);
        vty->buf_size += len;
        vty->cursor_pos = vty->buf_size;
        vty_write(vty, buf);
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
            vty->buf_size--;
            vty->buffer[vty->buf_size] = 0;
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
            vty_write(vty, "%s%s", variant_get_string(history_variant), VTY_NEWLINE(vty));
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

void    vty_new_line(vty_t* vty)
{
    vty_write(vty, VTY_NEWLINE(vty));
}

void    vty_set_command_received(vty_t* vty, bool is_cmd_received)
{
    vty->command_received = is_cmd_received;
}

bool    vty_is_command_received(vty_t* vty)
{
    return vty->command_received;
}

void    vty_shutdown(vty_t* vty)
{
    vty->shutdown = true;
}

bool    vty_is_shutdown(vty_t* vty)
{
    return vty->shutdown;
}

void    vty_set_authenticated(vty_t* vty, bool is_authenticated)
{
    vty->is_authenticated = is_authenticated;
}

bool    vty_is_authenticated(vty_t* vty)
{
    return vty->is_authenticated;
}

