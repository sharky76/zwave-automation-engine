#include "socket_io.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "cli_commands.h"
#include "stack.h"
#include "variant.h"
#include <logger.h>

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

void    socket_write_cb(vty_t* vty, const char* buf, size_t len)
{
    send(vty->data->desc.socket, buf, len, 0);
}

char*   socket_read_cb(vty_t* vty)
{
    int socket = vty->data->desc.socket;
    char ch[1] = {0};

    if(vty->multi_line)
    {
        vty_clear_buffer(vty);
        while(true)
        {
            // Read multiple lines of input into buffer until \n.\n is found
            if(recv(socket, ch, 1, 0) > 0)
            {
                if(ch[0] == EOF)
                {
                    return NULL;
                }
                
                if(ch[0] == KEY_BACKSPACE) // backspace
                {
                    vty_erase_char(vty);
                    continue;
                }
    
                if(ch[0] == vty->multiline_stop_char)
                {
                    break;
                }
            }
            vty_insert_char(vty, ch[0]);
        }
        return vty->buffer;
    }


    if(recv(socket, ch, 1, 0) > 0)
    {
        LOG_DEBUG(SocketIO, "Socket recv: %c (0x%x), buffer: %s, cursor: %d", ch[0], ch[0], vty->buffer, vty->cursor_pos);
        if(ch[0] == '?')
        {
            cli_command_describe_norl(vty);
            return NULL;
        }

        if(ch[0] == KEY_TAB) // tab
        {
            variant_stack_t* completions = cli_get_command_completions(vty, vty->buffer, vty->buf_size);
            
            if(NULL != completions)
            {
    
                if(completions->count == 1)
                {
                    variant_stack_t* cmd_stack = create_cmd_vec(vty->buffer);
                    variant_t* incomplete_cmd = stack_pop_back(cmd_stack);
                    variant_t* complete_cmd = stack_pop_front(completions);
    
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
                    stack_free(cmd_stack);
                    stack_free(completions);
                    return NULL;
                }
                else if(completions->count > 1)
                {
                    // See if we got another TAB - and display all possible completions
                    if(recv(socket, ch, 1, 0) > 0)
                    {
                        if(ch[0] == KEY_TAB)
                        {
                            int word_count = 0;
                            vty_write(vty, "\n");

                            stack_for_each(completions, matching_command)
                            {
                                vty_write(vty, "%-20s", variant_get_string(matching_command));
                                if(word_count++ >= 3)
                                {
                                    vty_write(vty, "\n");
                                    word_count = 0;
                                }
                            }
    
                            if(word_count > 0)
                            {
                                vty_write(vty, "\n");
                            }

                            vty_redisplay(vty, vty->buffer);
                            stack_free(completions);
                            return NULL;
                        }
                    }
                }
            }
            else
            {
                return NULL;
            }
        }

        if(ch[0] == KEY_BACKSPACE) // backspace
        {
            vty_erase_char(vty);
            return NULL;
        }

        if(ch[0] == KEY_ESC) // ESC char
        {
            char esc_sequence[2];
            if(recv(socket, esc_sequence, 2, 0) > 0)
            {   
                LOG_DEBUG(SocketIO, "ESC recv: %s (0x%x 0x%x)", esc_sequence, esc_sequence[0], esc_sequence[1]);
                if(esc_sequence[0] == KEY_BRACKET) // bracket char 
                {
                    switch(esc_sequence[1])
                    {
                    case KEY_UP_ARROW:
                        {
                            const char* hist = vty_get_history(vty, false);

                            if(NULL != hist)
                            {
                                vty_redisplay(vty, hist);
                            }
                            break;
                        }
                    case KEY_DOWN_ARROW:
                        {
                            const char* hist = vty_get_history(vty, true);
                            if(NULL == hist)
                            {
                                vty_clear_buffer(vty);
                            }
                            vty_redisplay(vty, hist);
                            break;
                        }
                    case KEY_LEFT_ARROW:
                        {
                            vty_cursor_left(vty);
                            break;
                        }
                    case KEY_RIGHT_ARROW:
                        {
                            vty_cursor_right(vty);
                            break;
                        }
                    default:
                        return NULL;
                    }
                }
            }

            return NULL;
        }

        if(ch[0] == KEY_NEWLINE)        
        {
            vty_add_history(vty);
            vty_append_char(vty, ch[0]);
            return vty->buffer;
        }
        else
        {
            vty_insert_char(vty, ch[0]);
        }

    }
    else
    {
        vty_set_error(vty, true);
    }

    return NULL;
}

void    socket_flush_cb(vty_t* vty)
{
    memset(vty->buffer, 0, vty->buf_size);
    vty->buf_size = 0;
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

