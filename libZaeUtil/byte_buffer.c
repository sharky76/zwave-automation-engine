#include "byte_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct byte_buffer_t
{
    char* buffer;
    int   read_pos;
    int   write_pos;
    int   length;
} byte_buffer_t;

byte_buffer_t* byte_buffer_init(int size)
{
    byte_buffer_t* buf = (byte_buffer_t*)calloc(1, sizeof(byte_buffer_t));
    buf->buffer = (char*)malloc(sizeof(char)*size);
    buf->length = size;

    return buf;
}

void           byte_buffer_free(byte_buffer_t* buffer)
{
    free(buffer->buffer);
    free(buffer);
}

void           byte_buffer_reset(byte_buffer_t* buffer)
{
    buffer->read_pos = buffer->write_pos = 0;
}

void           byte_buffer_grow(byte_buffer_t* buffer, int required_len)
{
    int adj_len = required_len - byte_buffer_spare_len(buffer);
    buffer->buffer = realloc(buffer->buffer, buffer->length + adj_len);
    buffer->length += adj_len;
}

void           byte_buffer_append(byte_buffer_t* buffer, const char* data, int len)
{
    if(byte_buffer_spare_len(buffer) <= len)
    {
        byte_buffer_grow(buffer, len);
    }

    memcpy(buffer->buffer + buffer->write_pos, data, len);
    buffer->write_pos += len;
}

void           byte_buffer_pack(byte_buffer_t* buffer)
{
    memmove(buffer->buffer, buffer->buffer + buffer->read_pos, buffer->length - buffer->read_pos);
    buffer->write_pos -= buffer->read_pos;
    buffer->read_pos = 0;
}

int            byte_buffer_read_len(byte_buffer_t* buffer)
{
    return buffer->write_pos - buffer->read_pos;
}

int            byte_buffer_spare_len(byte_buffer_t* buffer)
{
    return buffer->length - buffer->write_pos;
}

void           byte_buffer_adjust_read_pos(byte_buffer_t* buffer, int pos)
{
    buffer->read_pos += pos;
}

void           byte_buffer_adjust_write_pos(byte_buffer_t* buffer, int pos)
{
    buffer->write_pos += pos;
}

char*          byte_buffer_get_read_ptr(byte_buffer_t* buffer)
{
    return buffer->buffer + buffer->read_pos;
}

char*          byte_buffer_get_write_ptr(byte_buffer_t* buffer)
{
    return buffer->buffer + buffer->write_pos;
}

void           byte_buffer_vsnprintf(byte_buffer_t* buffer, const char* format, va_list args)
{
    int len = vsnprintf(buffer->buffer + buffer->write_pos, byte_buffer_spare_len(buffer), format, args);

    if(len >= byte_buffer_spare_len(buffer))
    {
        byte_buffer_grow(buffer, len+1);
        len = vsnprintf(buffer->buffer + buffer->write_pos, byte_buffer_spare_len(buffer), format, args);
    }

    byte_buffer_adjust_write_pos(buffer, len);
}