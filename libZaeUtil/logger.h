/*
This is logger utility header
*/
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum LogLevel_e
{
    LOG_LEVEL_NONE      = 0x0,
    LOG_LEVEL_ERROR     = 0x1,
    LOG_LEVEL_BASIC     = 0x2,
    LOG_LEVEL_ADVANCED  = 0x4,
    LOG_LEVEL_DEBUG     = 0x8
} LogLevel;

typedef enum LogTarget_e
{
    LOG_TARGET_FILE,
    LOG_TARGET_SYSLOG,
    LOG_TARGET_STDOUT
} LogTarget;

typedef struct file_logger_data_t
{
    char*   filename;
} file_logger_data_t;

typedef struct syslog_logger_data_t
{
    int priority;
} syslog_logger_data_t;

typedef struct stdout_logger_data_t
{
    FILE* fd;
} stdout_logger_data_t;

typedef struct logger_handle_t logger_handle_t;
extern logger_handle_t* logger_handle;

#define LOG_ERROR(_format_, ...)    \
    logger_log(logger_handle, LOG_LEVEL_ERROR, _format_, ##__VA_ARGS__)

#define LOG_INFO(_format_, ...) \
    logger_log(logger_handle, LOG_LEVEL_BASIC, _format_, ##__VA_ARGS__)

#define LOG_ADVANCED(_format_, ...) \
    logger_log(logger_handle, LOG_LEVEL_ADVANCED, _format_, ##__VA_ARGS__)

#define LOG_DEBUG(_format_, ...) \
    logger_log(logger_handle, LOG_LEVEL_DEBUG, _format_, ##__VA_ARGS__)

void logger_init(LogLevel level, LogTarget target, void* data);
void logger_log(logger_handle_t* handle, LogLevel level, const char* format, ...);
void logger_enable(bool enable);
void logger_set_level(LogLevel level);
bool logger_is_enabled();
LogLevel    logger_get_level();

