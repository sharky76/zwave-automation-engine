#include <stdarg.h>

typedef struct byte_buffer_t byte_buffer_t;

byte_buffer_t* byte_buffer_init(int size);
void           byte_buffer_free(byte_buffer_t* buffer);
void           byte_buffer_grow(byte_buffer_t* buffer, int required_len);
void           byte_buffer_append(byte_buffer_t* buffer, const char* data, int len);
void           byte_buffer_pack(byte_buffer_t* buffer);
int            byte_buffer_read_len(byte_buffer_t* buffer);
int            byte_buffer_spare_len(byte_buffer_t* buffer);
void           byte_buffer_adjust_read_pos(byte_buffer_t* buffer, int pos);
void           byte_buffer_adjust_write_pos(byte_buffer_t* buffer, int pos);
char*          byte_buffer_get_read_ptr(byte_buffer_t* buffer);
char*          byte_buffer_get_write_ptr(byte_buffer_t* buffer);
void           byte_buffer_vsnprintf(byte_buffer_t* buffer, const char* format, va_list args);