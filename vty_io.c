#include "vty_io.h"
//#include <readline/readline.h>
//#include <readline/history.h>
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
#include "socket.h"

bool    file_write_cb(vty_t* vty);
int     file_read_cb(vty_t* vty, char* str);
void    file_close_cb(vty_io_data_t* vty_data);

bool    std_write_cb(vty_t* vty);
int     std_read_cb(vty_t* vty, char* str);
void    std_erase_cb(vty_t* vty);
void    std_erase_line_cb(vty_t* vty);
void    std_cursor_left_cb(vty_t* vty);
void    std_cursor_right_cb(vty_t* vty);

int     http_read_cb(vty_t* vty, char* str);
bool    http_write_cb(vty_t* vty);
bool    http_flush_cb(vty_t* vty);
void    http_free_priv_cb(vty_t* vty);
void    socket_close_cb(vty_io_data_t* vty_data);

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
        vty->is_interactive = false;
        vty->data->close_cb = file_close_cb;

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
        vty->data->close_cb = NULL;

        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        vty->term_height = w.ws_row;
        vty->term_width = w.ws_col;
        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    case VTY_HTTP:
        vty->write_cb = NULL;
        vty->read_cb = http_read_cb;
        vty->flush_cb = http_flush_cb;
        vty->free_priv_cb = http_free_priv_cb;
        http_vty_priv_t* http_priv = calloc(1, sizeof(http_vty_priv_t));
        http_priv->request = calloc(1, sizeof(http_request_t));
        http_priv->request->buffer = byte_buffer_init(HTTP_REQUEST_BUFSIZE);
        //http_priv->response = byte_buffer_init(HTTP_REQUEST_BUFSIZE);
        http_priv->resp_code = HTTP_RESP_OK;
        http_priv->response_header = byte_buffer_init(HTTP_RESPONSE_HEADERS_SIZE);
        vty->priv = http_priv;
        vty->data->close_cb = socket_close_cb;

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
        vty->data->close_cb = socket_close_cb;

        //char iac_sga_buf[3] = {255, 251, 3};
        
        //socket_write_cb(vty, cmd_will_cr, 3);
        
        //vty->buffer = calloc(BUFSIZE, sizeof(char));
        break;
    }
}


bool    file_write_cb(vty_t* vty)
{
    int n =  fwrite(byte_buffer_get_read_ptr(vty->write_buffer), byte_buffer_read_len(vty->write_buffer), 1, vty->data->desc.file);

    if(n == 1)
    {
        byte_buffer_adjust_read_pos(vty->write_buffer, byte_buffer_read_len(vty->write_buffer));
        byte_buffer_pack(vty->write_buffer);
    }

    return n == 1;
}

int   file_read_cb(vty_t* vty, char* str)
{
    char ch = 0;
            
    //*str = malloc(sizeof(char));
    *str = fgetc(vty->data->desc.file);

    if(*str == (char)EOF)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

void    file_close_cb(vty_io_data_t* vty_data)
{
    fclose(vty_data->desc.file);
}

bool    std_write_cb(vty_t* vty)
{
    int n = write(fileno(vty->data->desc.io_pair[OUT]), byte_buffer_get_read_ptr(vty->write_buffer), byte_buffer_read_len(vty->write_buffer));
    if(n != -1)
    {
        fflush(vty->data->desc.io_pair[OUT]);
        byte_buffer_adjust_read_pos(vty->write_buffer, n);
        byte_buffer_pack(vty->write_buffer);
    }

    return n != -1;
}

int   std_read_cb(vty_t* vty, char* str)
{
    //*str = calloc(1, sizeof(char));

    return read(fileno(vty->data->desc.io_pair[IN]), str, 1);
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

int   http_read_cb(vty_t* vty, char* str)
{
    int socket = vty->data->desc.socket;
    http_vty_priv_t* http_priv = (http_vty_priv_t*)vty->priv;

    int ret = socket_recv(vty_get_pump(vty), socket, http_priv->request->buffer);

    if(-1 == ret)
    {
        event_dispatcher_unregister_handler(vty_get_pump(vty), vty->data->desc.socket, &vty_free, (void*)vty);
    }

    if(ret > 0)
    {
        int resp_len = http_server_read_request(http_priv, str);
        
        if(0 != resp_len && http_priv->resp_code == HTTP_RESP_OK)
        {
            vty_set_command_received(vty, true);
            return resp_len;
        }
        else if(http_priv->resp_code != HTTP_RESP_OK)
        {
            vty_set_command_received(vty, true);
        }
    }

    return 0;
}

bool    http_flush_cb(vty_t* vty)
{
    bool retVal = false;
    http_vty_priv_t* http_priv = (http_vty_priv_t*)vty->priv;
    int socket = vty->data->desc.socket;

    http_server_prepare_response_headers(http_priv, byte_buffer_read_len(vty->write_buffer));

    struct iovec iov[2];
    iov[0].iov_base = byte_buffer_get_read_ptr(http_priv->response_header);
    iov[0].iov_len = byte_buffer_read_len(http_priv->response_header);

    iov[1].iov_base = byte_buffer_get_read_ptr(vty->write_buffer);
    iov[1].iov_len = byte_buffer_read_len(vty->write_buffer);


    int ret = socket_send_v(vty_get_pump(vty), socket, iov, 2, byte_buffer_read_len(http_priv->response_header) + byte_buffer_read_len(vty->write_buffer));
    
    if(-1 == ret)
    {
        //printf("HTTP send failed, unregestering socket!\n");
        event_dispatcher_unregister_handler(vty_get_pump(vty), vty->data->desc.socket, &vty_free, (void*)vty);
    }
    else if(ret == byte_buffer_read_len(http_priv->response_header) + byte_buffer_read_len(vty->write_buffer))
    {
        byte_buffer_adjust_read_pos(vty->write_buffer, byte_buffer_read_len(vty->write_buffer));
        byte_buffer_pack(vty->write_buffer);
        byte_buffer_adjust_read_pos(http_priv->response_header, byte_buffer_read_len(http_priv->response_header));
        byte_buffer_pack(http_priv->response_header);

        http_set_response(http_priv, HTTP_RESP_NONE);
        vty->buf_size = 0;
        *vty->buffer = 0;
        retVal = true;

        //printf("HTTP send complete: %d\n", ret);
    }
    else 
    {
        //printf("Partial data sent: %d out of %d\n", ret, byte_buffer_read_len(http_priv->response_header) + byte_buffer_read_len(vty->write_buffer));
        if(ret >= byte_buffer_read_len(http_priv->response_header))
        {
            int remaining = ret - byte_buffer_read_len(http_priv->response_header);

            if(byte_buffer_read_len(http_priv->response_header) > 0)
            {
                byte_buffer_adjust_read_pos(http_priv->response_header, byte_buffer_read_len(http_priv->response_header));
            }

            if(byte_buffer_read_len(vty->write_buffer) > 0)
            {
                byte_buffer_adjust_read_pos(vty->write_buffer, remaining);
                byte_buffer_pack(vty->write_buffer);
            }

            http_set_response(http_priv, HTTP_RESP_NONE);
        }
        else
        {
            byte_buffer_adjust_read_pos(http_priv->response_header, ret);
        }

        retVal = true;
    }


    /*
    int ret = socket_send_v(vty_get_pump(vty), socket, iov, 2, byte_buffer_read_len(http_priv->response_header) + byte_buffer_read_len(http_priv->response));
    
    if(-1 == ret)
    {
        printf("HTTP send failed, unregestering socket!\n");
        event_dispatcher_unregister_handler(vty_get_pump(vty), vty->data->desc.socket, &vty_free, (void*)vty);
    }
    else if(ret == byte_buffer_read_len(http_priv->response_header) + byte_buffer_read_len(http_priv->response))
    {
        byte_buffer_adjust_read_pos(http_priv->response, byte_buffer_read_len(http_priv->response));
        byte_buffer_pack(http_priv->response);
        byte_buffer_adjust_read_pos(http_priv->response_header, byte_buffer_read_len(http_priv->response_header));
        byte_buffer_pack(http_priv->response_header);

        http_set_response(http_priv, HTTP_RESP_NONE);
        vty->buf_size = 0;
        *vty->buffer = 0;
        retVal = true;

        printf("HTTP send complete\n");
    }
    else 
    {
        printf("Partial data sent: %d out of %d\n", ret, byte_buffer_read_len(http_priv->response_header) + byte_buffer_read_len(http_priv->response));
        if(ret >= byte_buffer_read_len(http_priv->response_header))
        {
            int remaining = ret - byte_buffer_read_len(http_priv->response_header);

            if(byte_buffer_read_len(http_priv->response_header) > 0)
            {
                byte_buffer_adjust_read_pos(http_priv->response_header, byte_buffer_read_len(http_priv->response_header));
            }

            if(byte_buffer_read_len(http_priv->response) > 0)
            {
                byte_buffer_adjust_read_pos(http_priv->response, remaining);
            }

            http_set_response(http_priv, HTTP_RESP_NONE);
        }
        else
        {
            byte_buffer_adjust_read_pos(http_priv->response_header, ret);
        }

        retVal = true;
    }
    */
    
    
    return retVal;
}

void    http_free_priv_cb(vty_t* vty)
{
    http_vty_priv_t* http_priv = (http_vty_priv_t*)vty->priv;

    byte_buffer_free(http_priv->request->buffer);
    free(http_priv->request);
    //byte_buffer_free(http_priv->response);
    free(http_priv->content_type);
    free(http_priv->post_data);
    free(http_priv->headers);
    byte_buffer_free(http_priv->response_header);
    free(http_priv);
}
