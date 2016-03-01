#pragma once

/*
This is logger utility header
*/
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum LogLevel_e
{
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

// Registered services table
typedef struct logger_service_t
{
    int     service_id;
    char*   service_name;
    bool    enabled;
    LogLevel level;
} logger_service_t;

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

#define LOG_ERROR(_id_, _format_, ...)    \
    logger_log(logger_handle, _id_, LOG_LEVEL_ERROR, _format_, ##__VA_ARGS__)

#define LOG_INFO(_id_, _format_, ...) \
    logger_log(logger_handle, _id_, LOG_LEVEL_BASIC, _format_, ##__VA_ARGS__)

#define LOG_ADVANCED(_id_, _format_, ...) \
    logger_log(logger_handle, _id_, LOG_LEVEL_ADVANCED, _format_, ##__VA_ARGS__)

#define LOG_DEBUG(_id_, _format_, ...) \
    logger_log_with_func(logger_handle, _id_, LOG_LEVEL_DEBUG, __func__, _format_, ##__VA_ARGS__)

#define USING_LOGGER(_logger_)  \
    extern int _logger_;

#define DECLARE_LOGGER(_logger_)   \
    int _logger_; \
    void LOG_##_logger_##_register() { logger_register_service(&_logger_, #_logger_); } \

#define REGISTER_LOGGER(_logger_)   \
    extern void LOG_##_logger_##_register(); \
    LOG_##_logger_##_register();

void logger_init(LogLevel level, LogTarget target, void* data);

void logger_register_service(int* serviceId, const char* name);
void logger_register_service_with_id(int serviceId, const char* name);
void logger_enable_service(int serviceId);
void logger_disable_service(int serviceId);
bool logger_get_services(logger_service_t*** services, int* size);

void logger_log(logger_handle_t* handle, int id, LogLevel level, const char* format, ...);
void logger_log_with_func(logger_handle_t* handle, int id, LogLevel level, const char* func, const char* format, ...);
void logger_enable(bool enable);
void logger_set_level(int serviceId, LogLevel level);
bool logger_is_enabled();
LogLevel    logger_get_level(int serviceId);
void logger_set_buffer(int size);
int  logger_get_buffer_size();
void logger_set_console(bool is_console);
void logger_print_buffer();




