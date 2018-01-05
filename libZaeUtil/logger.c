#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "vty.h"
#include <pthread.h>

#define MAX_LOG_ENTRY_LEN   512

typedef struct logger_vty_t
{
    vty_t* vty;
    bool   is_online;
} logger_vty_t;

typedef struct logger_log_entry_t
{
    logger_service_t*   service;
    LogLevel            level;
    char*               line;
} logger_log_entry_t;

void    delete_logger_log_entry(void* arg)
{
    logger_log_entry_t* entry = (logger_log_entry_t*)arg;
    free(entry->line);
    free(entry);
};

// Global handle
typedef struct logger_handle_t
{
    variant_stack_t*    registered_targets;

    bool        enabled;
    hash_table_t*   logger_service_table;
    int log_buffer_size;
    variant_stack_t*    log_buffer;
    pthread_mutex_t     logger_lock;
} logger_handle_t;

logger_handle_t*    logger_handle = NULL;

void    valogger_log(logger_handle_t* handle, logger_service_t* service, LogLevel level, const char* format, va_list args);

void    logger_add_buffer(logger_log_entry_t* entry);

void logger_init()
{
    logger_handle = (logger_handle_t*)malloc(sizeof(logger_handle_t));
    logger_handle->enabled = true;
    logger_handle->logger_service_table = variant_hash_init();
    logger_handle->log_buffer_size = 100;
    logger_handle->log_buffer = stack_create();
    logger_handle->registered_targets = stack_create();

    pthread_mutex_init(&logger_handle->logger_lock, NULL);
}

void logger_register_target(vty_t* vty)
{
    logger_vty_t* new_vty = malloc(sizeof(logger_vty_t));
    new_vty->vty = vty;
    new_vty->is_online = false;
    stack_push_back(logger_handle->registered_targets, variant_create_ptr(DT_PTR, new_vty, NULL));
}

void logger_unregister_target(vty_t* vty)
{
    stack_for_each(logger_handle->registered_targets, log_target_variant)
    {
        logger_vty_t* target_vty = VARIANT_GET_PTR(logger_vty_t, log_target_variant);
        if(target_vty->vty == vty)
        {
            stack_remove(logger_handle->registered_targets, log_target_variant);
            variant_free(log_target_variant);
            break;
        }
    }
}

void valogger_log(logger_handle_t* handle, logger_service_t* service, LogLevel level, const char* format, va_list args)
{
    if(NULL != service && service->enabled == true &&
       service->level >= level && handle->enabled)
    {
        time_t t = time(NULL);
        struct tm* ptime = localtime(&t);
        char time_str[15] = {0};

        strftime(time_str, sizeof(time_str), "%H:%M:%S ", ptime);
        char log_format_buffer[MAX_LOG_ENTRY_LEN] = {0};
        strcat(log_format_buffer, time_str);
        strcat(log_format_buffer, "%%");
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

        strncat(log_format_buffer, format, MAX_LOG_ENTRY_LEN-len_so_far-2);
        strcat(log_format_buffer, "\r\n");

        char buf[1024] = {0};
        vsnprintf(buf, 1023, log_format_buffer, args);

        logger_log_entry_t* new_entry = calloc(1, sizeof(logger_log_entry_t));
        new_entry->service = service;
        new_entry->level = level;
        new_entry->line = strdup(buf);


        logger_add_buffer(new_entry);
        //handle->do_log(handle, buf);

        stack_for_each(handle->registered_targets, vty_variant)
        {
            logger_vty_t* logger_vty = VARIANT_GET_PTR(logger_vty_t, vty_variant);

            if(logger_vty->is_online)
            {
                vty_write(logger_vty->vty, "%s", buf);
            }
        }
    }
}

void logger_log(logger_handle_t* handle, int id, LogLevel level, const char* format, ...)
{
    pthread_mutex_lock(&handle->logger_lock);
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
        //printf("Unregistered module id: %d\n", id);
    }

    pthread_mutex_unlock(&handle->logger_lock);
}

void logger_log_with_func(logger_handle_t* handle, int id, LogLevel level, const char* func, const char* format, ...)
{
    pthread_mutex_lock(&handle->logger_lock);
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
    pthread_mutex_unlock(&handle->logger_lock);
}

void    logger_add_buffer(logger_log_entry_t* entry)
{
    while(logger_handle->log_buffer->count > logger_handle->log_buffer_size)
    {
        variant_t* log_var = stack_pop_front(logger_handle->log_buffer);
        variant_free(log_var);
    }

    if(logger_handle->log_buffer_size > 0)
    {

        stack_push_back(logger_handle->log_buffer, variant_create_ptr(DT_PTR, entry, &delete_logger_log_entry));
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
    new_service->level = LOG_LEVEL_BASIC;
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

void logger_set_online(vty_t* vty, bool is_online)
{
    stack_for_each(logger_handle->registered_targets, log_target_variant)
    {
        logger_vty_t* target_vty = VARIANT_GET_PTR(logger_vty_t, log_target_variant);
        if(target_vty->vty == vty)
        {
            target_vty->is_online = is_online;
            break;
        }
    }
}

void logger_print_buffer(vty_t* vty)
{
    pthread_mutex_lock(&logger_handle->logger_lock);
    stack_for_each(logger_handle->log_buffer, log_variant)
    {
        logger_log_entry_t* entry = VARIANT_GET_PTR(logger_log_entry_t, log_variant);
        vty_write(vty, "%s", entry->line);
    }
    pthread_mutex_unlock(&logger_handle->logger_lock);
}

void logger_clear_buffer()
{
    while(logger_handle->log_buffer->count > 0)
    {
        variant_t* entry = stack_pop_front(logger_handle->log_buffer);
        variant_free(entry);
    }
}

