#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <string.h>

#define MAX_LOG_ENTRY_LEN   512

// Global handle
typedef struct logger_handle_t
{
    LogLevel    level;
    LogTarget   target;
    bool        enabled;
    FILE*       fh;

    void    (*do_log)(struct logger_handle_t*, const char*, va_list);

    union {
        file_logger_data_t*      file;
        syslog_logger_data_t*    syslog;
        stdout_logger_data_t*    stdout;
    } data;
} logger_handle_t;

logger_handle_t*    logger_handle = NULL;

void file_logger_data_init(logger_handle_t* handle);
void syslog_logger_data_init(logger_handle_t* handle);
void stdout_logger_data_init(logger_handle_t* handle);

void    do_file_log(logger_handle_t* handle, const char* format, va_list args);
void    do_syslog_log(logger_handle_t* handle, const char* format, va_list args);
void    do_stdout_log(logger_handle_t* handle, const char* format, va_list args);

void logger_init(LogLevel level, LogTarget target, void* data)
{
    logger_handle = (logger_handle_t*)malloc(sizeof(logger_handle_t));

    logger_handle->level = level;
    logger_handle->target = target;
    logger_handle->enabled = true;

    switch(target)
    {
    case LOG_TARGET_FILE:
        logger_handle->data.file = (file_logger_data_t*)data;
        file_logger_data_init(logger_handle);
        break;
    case LOG_TARGET_SYSLOG:
        logger_handle->data.syslog = (syslog_logger_data_t*)data;
        syslog_logger_data_init(logger_handle);
        break;
    case LOG_TARGET_STDOUT:
        logger_handle->data.stdout = (stdout_logger_data_t*)data;
        stdout_logger_data_init(logger_handle);
        break;
    }
}

void logger_log(logger_handle_t* handle, LogLevel level, const char* format, ...)
{
    if(handle->level >= level && handle->enabled)
    {
        time_t t = time(NULL);
        struct tm* ptime = localtime(&t);
        char time_str[10] = {0};

        strftime(time_str, sizeof(time_str), "%H:%M:%S", ptime);
        char log_format_buffer[MAX_LOG_ENTRY_LEN] = {0};
        strcat(log_format_buffer, time_str);
        strcat(log_format_buffer, " ");

        switch(level)
        {
        case LOG_LEVEL_BASIC:
            strcat(log_format_buffer, "[INFO] ");
            break;
        case LOG_LEVEL_ERROR:
            strcat(log_format_buffer, "[ERROR] ");
            break;
        case LOG_LEVEL_ADVANCED:
            strcat(log_format_buffer, "[ADV] ");
            break;
        case LOG_LEVEL_DEBUG:
            strcat(log_format_buffer, "[DEBUG] ");
            break;
        }

        strncat(log_format_buffer, format, MAX_LOG_ENTRY_LEN-21);
        strcat(log_format_buffer, "\n");
        va_list args;
        va_start(args, format);
        handle->do_log(handle, log_format_buffer, args);
    }
}

void logger_set_level(LogLevel level)
{
    logger_handle->level = level;
}

void logger_enable(bool enable)
{
    logger_handle->enabled = enable;
}

void file_logger_data_init(logger_handle_t* handle)
{
    handle->fh = fopen(handle->data.file->filename, "a+");
    handle->do_log = &do_file_log;
}

void syslog_logger_data_init(logger_handle_t* handle)
{
    openlog("ZAE", LOG_PID, LOG_USER);
    handle->do_log = &do_syslog_log;
}

void stdout_logger_data_init(logger_handle_t* handle)
{
    handle->fh = handle->data.stdout->fd;
    handle->do_log = &do_file_log;
}

void    do_file_log(logger_handle_t* handle, const char* format, va_list args)
{
    vfprintf(handle->fh, format, args);
}

void    do_syslog_log(logger_handle_t* handle, const char* format, va_list args)
{
    vsyslog(handle->data.syslog->priority, format, args);
}

