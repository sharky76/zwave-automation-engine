#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"

#define MAX_LOG_ENTRY_LEN   512

// Global handle
typedef struct logger_handle_t
{
    LogLevel    level;
    LogTarget   target;
    bool        enabled;
    FILE*       fh;
    int         fd;
    hash_table_t*   logger_service_table;

    bool        console;

    int log_buffer_size;
    variant_stack_t*    log_buffer;

    void    (*do_log)(struct logger_handle_t*, const char*);

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

void    do_file_log(logger_handle_t* handle, const char* line);
void    do_syslog_log(logger_handle_t* handle, const char* line);
void    do_stdout_log(logger_handle_t* handle, const char* line);
void    valogger_log(logger_handle_t* handle, logger_service_t* service, LogLevel level, const char* format, va_list args);

void    logger_add_buffer(const char* line);

void logger_init(LogLevel level, LogTarget target, void* data)
{
    logger_handle = (logger_handle_t*)malloc(sizeof(logger_handle_t));

    logger_handle->level = level;
    logger_handle->target = target;
    logger_handle->enabled = true;
    
    logger_handle->logger_service_table = variant_hash_init();

    logger_handle->log_buffer_size = 100;
    logger_handle->log_buffer = stack_create();

    logger_handle->console = false;

    logger_set_data(data);
}

void logger_set_data(void* data)
{
    switch(logger_handle->target)
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

void valogger_log(logger_handle_t* handle, logger_service_t* service, LogLevel level, const char* format, va_list args)
{
    if(NULL != service && service->enabled == true &&
       service->level >= level && handle->enabled)
    {
        time_t t = time(NULL);
        struct tm* ptime = localtime(&t);
        char time_str[12] = {0};

        strftime(time_str, sizeof(time_str), "%H:%M:%S %%%", ptime);
        char log_format_buffer[MAX_LOG_ENTRY_LEN] = {0};
        strcat(log_format_buffer, time_str);
        strcat(log_format_buffer, service->service_name);
        strcat(log_format_buffer, "-");

        switch(level)
        {
        case LOG_LEVEL_BASIC:
            strcat(log_format_buffer, "INFO: ");
            break;
        case LOG_LEVEL_ERROR:
            strcat(log_format_buffer, "ERROR: ");
            break;
        case LOG_LEVEL_ADVANCED:
            strcat(log_format_buffer, "ADVANCED: ");
            break;
        case LOG_LEVEL_DEBUG:
            strcat(log_format_buffer, "DEBUG: ");
            break;
        default:
            return;
        }

        int len_so_far = strlen(log_format_buffer);

        strncat(log_format_buffer, format, MAX_LOG_ENTRY_LEN-len_so_far-1);
        strcat(log_format_buffer, "\n");

        char buf[1024] = {0};
        vsnprintf(buf, 1023, log_format_buffer, args);
        logger_add_buffer(buf);
        handle->do_log(handle, buf);
    }
}

void logger_log(logger_handle_t* handle, int id, LogLevel level, const char* format, ...)
{
    // First check if the service id is enabled
    variant_t* service_variant = variant_hash_get(logger_handle->logger_service_table, id);

    if(NULL != service_variant)
    {
        logger_service_t* service = (logger_service_t*)variant_get_ptr(service_variant);
        va_list args;
        va_start(args, format);
        valogger_log(handle, service, level, format, args);
    }
    else
    {
        printf("Unregistered module id: %d\n", id);
    }
}

void logger_log_with_func(logger_handle_t* handle, int id, LogLevel level, const char* func, const char* format, ...)
{
    // First check if the service id is enabled
    variant_t* service_variant = variant_hash_get(logger_handle->logger_service_table, id);

    if(NULL != service_variant)
    {
        logger_service_t* service = (logger_service_t*)variant_get_ptr(service_variant);
    
        char* new_format = calloc(strlen(format) + strlen(func) + 5, sizeof(char));
        strcat(new_format, func);
        strcat(new_format, "(): ");
        strcat(new_format, format);
    
        va_list args;
        va_start(args, format);
        valogger_log(handle, service, level, new_format, args);
        free(new_format);
    }
    else
    {
        printf("Unregistered module id: %d\n", id);
    }
}

void    logger_add_buffer(const char* line)
{
    while(logger_handle->log_buffer->count > logger_handle->log_buffer_size)
    {
        variant_t* log_var = stack_pop_front(logger_handle->log_buffer);
        variant_free(log_var);
    }

    if(logger_handle->log_buffer_size > 0)
    {
        stack_push_back(logger_handle->log_buffer, variant_create_string(strdup(line)));
    }
}

void logger_set_level(int serviceId, LogLevel level)
{
    variant_t* service_variant = variant_hash_get(logger_handle->logger_service_table, serviceId);
    logger_service_t* service = (logger_service_t*)variant_get_ptr(service_variant);
    service->level = level;
    service->enabled = true;
}

void logger_enable(bool enable)
{
    logger_handle->enabled = enable;
}

bool logger_is_enabled()
{
    return logger_handle->enabled;
}

LogLevel    logger_get_level(int serviceId)
{
    variant_t* service_variant = variant_hash_get(logger_handle->logger_service_table, serviceId);
    logger_service_t* service = (logger_service_t*)variant_get_ptr(service_variant);
    return service->level;
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
    //handle->fh = fdopen(handle->data.stdout->fd, "rw");
    handle->fd = handle->data.stdout->fd;
    handle->do_log = &do_stdout_log;
}

void    do_file_log(logger_handle_t* handle, const char* line)
{
   //fprintf(handle->fh, "%s", line);
    write(handle->fd, line, strlen(line));
}

void    do_stdout_log(logger_handle_t* handle, const char* line)
{
    if(handle->console)
    {
        do_file_log(handle, line);
    }
}

void    do_syslog_log(logger_handle_t* handle, const char* line)
{
    syslog(handle->data.syslog->priority, line);
}

void logger_register_service(int* serviceId, const char* name)
{
    *serviceId = variant_get_next_user_type();
    logger_register_service_with_id(*serviceId, name);
}

void logger_register_service_with_id(int serviceId, const char* name)
{
    logger_service_t* new_service = calloc(1, sizeof(logger_service_t));
    new_service->service_id = serviceId;
    new_service->service_name = strdup(name);
    new_service->enabled = true;
    new_service->level = logger_handle->level;
    variant_hash_insert(logger_handle->logger_service_table, serviceId, variant_create_ptr(DT_PTR, new_service, variant_delete_default));
}


void logger_enable_service(int serviceId)
{
    variant_t* service_variant = variant_hash_get(logger_handle->logger_service_table, serviceId);
    logger_service_t* service = (logger_service_t*)variant_get_ptr(service_variant);
    service->enabled = true;
}

void logger_disable_service(int serviceId)
{
    variant_t* service_variant = variant_hash_get(logger_handle->logger_service_table, serviceId);
    logger_service_t* service = (logger_service_t*)variant_get_ptr(service_variant);
    service->enabled = false;
}

/*
 
    Helper method for traversing service hash... 
 
*/
typedef struct add_service_context_t
{
    logger_service_t** service_array;
    int                current_index;
} add_service_context_t;

void add_service(hash_node_data_t* hash_node_data, void* context);

bool logger_get_services(logger_service_t*** services, int* size)
{
    // Create array of pointers
    *services = calloc(logger_handle->logger_service_table->count, sizeof(logger_service_t*));

    add_service_context_t service_context = {
        .current_index = 0,
        .service_array = *services
    };

    variant_hash_for_each(logger_handle->logger_service_table, add_service, (void*)&service_context);
    *size = logger_handle->logger_service_table->count;

    return true;
}

void add_service(hash_node_data_t* hash_node_data, void* context)
{
    add_service_context_t* service_context = (add_service_context_t*)context;
    logger_service_t* service = (logger_service_t*)variant_get_ptr(hash_node_data->data);
    
    //printf("%d service: %s\n", service->service_id, service->service_name);

    *(service_context->service_array) = service;
    service_context->service_array ++;
}

void logger_set_buffer(int size)
{
    logger_handle->log_buffer_size = size;
}

int  logger_get_buffer_size()
{
    return logger_handle->log_buffer_size;
}

void logger_set_console(bool is_console)
{
    logger_handle->console = is_console;
}

void logger_print_buffer()
{
    bool saved_console = logger_handle->console;
    logger_handle->console = true;

    stack_for_each(logger_handle->log_buffer, log_variant)
    {
        logger_handle->do_log(logger_handle, variant_get_string(log_variant));
    }

    logger_handle->console = saved_console;
}

void logger_clear_buffer()
{
    while(logger_handle->log_buffer->count > 0)
    {
        variant_t* line = stack_pop_front(logger_handle->log_buffer);
        variant_free(line);
    }
}

