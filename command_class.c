#include "command_class.h"
#include "data_callbacks.h"
#include <logger.h>
#include "event.h"
#include "hash.h"
#include "crc32.h"

extern ZWay zway;
extern hash_table_t*   data_holder_event_table;

#define COMMAND_DATA_READY_EVENT    "CommandDataReady"

USING_LOGGER(DataCallback)

variant_t*   command_class_eval_basic(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_binarysensor(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_battery(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_alarm(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_binaryswitch(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_configuration(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_manufacturer_specific(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_wakeup(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_sensor_multilevel(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_indicator(const char* method, device_record_t* record, va_list args);

typedef struct zway_data_read_ctx_t
{
    device_record_t*        record;
} zway_data_read_ctx_t;

void         zway_data_read_success_cb(const ZWay zway, ZWBYTE functionId, void* arg);
void         zway_data_read_fail_cb(const ZWay zway, ZWBYTE functionId, void* arg);

static command_class_t command_class_table[] = {
    {0x20, "Basic",        {"Get", 0, "", "Set", 1, "level", NULL, 0, NULL},       &command_class_eval_basic},
    {0x30, "SensorBinary", {"Get", 1, "<sensor>,<parameter>", NULL, 0, NULL},                        &command_class_eval_binarysensor},
    {0x80, "Battery",      {"Get", 0, "", NULL, 0, NULL},                           &command_class_eval_battery},
    {0x71, "Alarm",        {"Get", 1, "<type>.<field>",  "Set", 2, "<type>, <level>", NULL, 0, NULL},  &command_class_eval_alarm},
    {0x25, "SwitchBinary", {"Get", 0, "",  "Set", 1, "Bool", NULL, 0, NULL},              &command_class_eval_binaryswitch},
    {0x70, "Configuration", {"Get", 1, "<parameter>", "Set", 3, "<parameter>, <value>, <size>", NULL, 0, NULL},       &command_class_eval_configuration},
    {0x72, "ManufacturerSpecific", {"Get", 0, "", NULL, 0, NULL},                                     &command_class_eval_manufacturer_specific},
    {0x84, "Wakeup",       {"Get", 0, "", "Capabilities", 0, "", "Sleep", 0, "", "Set", 2, "<seconds>, <node ID>", NULL, 0, NULL},       &command_class_eval_wakeup},
    {0x31, "SensorMultilevel",       {"Get", 1, "<sensor>", NULL, 0, NULL},       &command_class_eval_sensor_multilevel},
    {0x81, "Indicator",     {"Get", 0, "", "Set", 1, "<parameter>", NULL, 0, NULL}, &command_class_eval_indicator},  

    /* other standard command classes */
    {0, NULL,   {NULL, 0, NULL},   NULL}
};

void         zway_data_read_success_cb(const ZWay zway, ZWBYTE functionId, void* arg)
{
    zway_data_read_ctx_t* ctx = (zway_data_read_ctx_t*)arg;

    LOG_DEBUG(DataCallback, "Data read success for device %s with function 0x%x",
              ctx->record->deviceName,
              functionId);

    event_t* event = event_create(DataCallback, COMMAND_DATA_READY_EVENT, NULL);
    event_post(event);

    free(ctx);
}

void         zway_data_read_fail_cb(const ZWay zway, ZWBYTE functionId, void* arg)
{
    zway_data_read_ctx_t* ctx = (zway_data_read_ctx_t*)arg;
    LOG_DEBUG(DataCallback, "Data read failure for device %s with function 0x%x",
              ctx->record->deviceName,
              functionId);

    event_t* event = event_create(DataCallback, COMMAND_DATA_READY_EVENT, NULL);
    event_post(event);

    free(ctx);
}


command_class_t*    get_command_class_by_id(ZWBYTE command_id)
{
    command_class_t* retVal = NULL;
    int i = 0;
    for(i = 0; i < MAX_COMMAND_CLASSES; i++)
    {
        retVal = &command_class_table[i];
        if(retVal->command_id == command_id)
        {
            return retVal;
        }
        else if(retVal->command_id == 0)
        {
            return NULL;
        }
    }
}

command_class_t*    get_command_class_by_name(const char* command_name)
{
    command_class_t* retVal = NULL;
    int i = 0;
    for(i = 0; i < MAX_COMMAND_CLASSES; i++)
    {
        retVal = &command_class_table[i];
        if(strncmp(retVal->command_name, command_name, MAX_COMMAND_NAME_LEN) == 0)
        {
            return retVal;
        }
        else if(retVal->command_id == 0)
        {
            return NULL;
        }
    }
}

void    command_class_for_each(void (*visitor)(command_class_t*, void*), void* arg)
{
    int i = 0;
    command_class_t* command_class = &command_class_table[i++];

    while(command_class->command_id != 0)
    {
        visitor(command_class, arg);
        command_class = &command_class_table[i++];
    }
}

variant_t*  command_class_read_data(device_record_t* record, const char* path)
{
    variant_t* ret_val = NULL;
    zdata_acquire_lock(ZDataRoot(zway));
    ZDataHolder dh = zway_find_device_instance_cc_data(zway, record->nodeId, record->instanceId, record->commandId, path);

    ZWDataType type;
    zdata_get_type(dh, &type);

    //printf("Found zdata type %d\n", type);
    switch(type)
    {
    case Empty:
        {
            ret_val = variant_create_string(strdup("Empty"));
        }
        break;
    case Integer:
        {
            int int_val;
            zdata_get_integer(dh, &int_val);
            ret_val = variant_create_int32(DT_INT32, int_val);
        }
        break;
    case Float:
        {
            float float_val;
            zdata_get_float(dh, &float_val);
            ret_val = variant_create_float(float_val);
        }
        break;
    case String:
        {
            const char* string_val;
            zdata_get_string(dh, &string_val);
            ret_val = variant_create_string(strdup(string_val));
        }
        break;
    case Boolean:
        {
            ZWBOOL bool_val;
            zdata_get_boolean(dh, &bool_val);
            ret_val = variant_create_bool((bool)bool_val);
        }
        break;
    default:
        break;

    }
    zdata_release_lock(ZDataRoot(zway));
    return ret_val;
}

variant_t*   command_class_eval_basic(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);
        /*zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_basic_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx); 
        */
                                                                                                                                  ;
        /*zdata_acquire_lock(ZDataRoot(zway));
        ZDataHolder dh = zway_find_device_instance_data(zway, record->nodeId, record->instanceId, "level");
        int int_val;
        zdata_get_integer(dh, &int_val);
        ret_val = variant_create_int32(DT_INT32, int_val);
        zdata_release_lock(ZDataRoot(zway));*/
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        ZWError err = zway_cc_basic_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }
        ret_val = command_class_read_data(record, variant_get_string(arg1));
    }
    else if(strcmp(method, "Set") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);
        ZWError err = zway_cc_basic_set(zway, record->nodeId, record->instanceId, variant_get_byte(arg1), NULL, NULL, NULL);
        ret_val = variant_create_bool(err == NoError);
    }

    return ret_val;
}

variant_t*   command_class_eval_binarysensor(const char* method, device_record_t* record, va_list args)
{
    
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);

        /*ZWBOOL bool_val;
        zdata_get_boolean(dh, &bool_val);
        ret_val = variant_create_bool((bool)bool_val);
        */
        /*zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        ZWError err = zway_cc_sensor_binary_get(zway, record->nodeId, record->instanceId, -1, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        if(err != NoError)
        {
            LOG_ERROR(DataCallback, "Unable to get binary sensor %s data", record->deviceName);
        }
        else
        {
            LOG_DEBUG(DataCallback, "Binary sensor GET sent");
        }*/

        ret_val = command_class_read_data(record, variant_get_string(arg1));
    }

    return ret_val;
}

variant_t*   command_class_eval_battery(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_battery_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        ret_val = command_class_read_data(record, "last");
    }

    return ret_val;
}

variant_t*   command_class_eval_alarm(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_alarm_get(zway, record->nodeId, record->instanceId, 0, 0, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        // Block here...
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }

        ret_val = command_class_read_data(record, variant_get_string(arg1));
    }
    else if(strcmp(method, "Set") == 0)
    {
        variant_t* alarm_type = va_arg(args, variant_t*);
        variant_t* level = va_arg(args, variant_t*);
        ZWError err = zway_cc_alarm_set(zway,record->nodeId, record->instanceId, variant_get_int(alarm_type), variant_get_int(level), NULL, NULL, NULL);
        ret_val = variant_create_bool(err == NoError);
    }

    return ret_val;
}

variant_t*   command_class_eval_binaryswitch(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_switch_binary_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);

        // Block here...
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }

        ret_val = command_class_read_data(record, "level");
    }
    else if(strcmp(method, "Set") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);

        LOG_ADVANCED(DataCallback, "Setting device %s to %d", record->deviceName, variant_get_bool(arg1));
        ZWError err = zway_cc_switch_binary_set(zway,record->nodeId, record->instanceId, variant_get_bool(arg1), NULL, NULL, NULL);
        ret_val = variant_create_bool(err == NoError);
    }

    return ret_val;
}

variant_t*   command_class_eval_configuration(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;
    variant_t* param = va_arg(args, variant_t*);

    if(strcmp(method, "Get") == 0)
    {
        char* param_str;
        variant_to_string(param, &param_str);
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_configuration_get(zway, record->nodeId, record->instanceId, variant_get_int(param), zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        // Block here...
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }

        ret_val = command_class_read_data(record, param_str);
        free(param_str);
    }
    else if(strcmp(method, "Set") == 0)
    {
        variant_t* value = va_arg(args, variant_t*);
        variant_t* size = va_arg(args, variant_t*);

        ZWError err = zway_cc_configuration_set(zway,record->nodeId, record->instanceId, variant_get_int(param), variant_get_int(value), variant_get_int(size), NULL, NULL, NULL);
        ret_val = variant_create_bool(err == NoError);
    }

    return ret_val;
}

variant_t*   command_class_eval_manufacturer_specific(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_manufacturer_specific_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        // Block here...
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }

        ret_val = command_class_read_data(record, NULL);
    }

    return ret_val;
}

variant_t*   command_class_eval_wakeup(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_wakeup_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        // Block here...
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }
        ret_val = command_class_read_data(record, NULL);
    }
    else if(strcmp(method, "Capabilities") == 0)
    {
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_wakeup_capabilities_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        // Block here...
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }
        ret_val = command_class_read_data(record, NULL);
    }
    else if(strcmp(method, "Sleep") == 0)
    {
        ZWError err = zway_cc_wakeup_sleep(zway, record->nodeId, record->instanceId, NULL, NULL, NULL);
        ret_val = variant_create_bool(err == NoError);
    }
    else if(strcmp(method, "Set") == 0)
    {
        variant_t* interval = va_arg(args, variant_t*);
        variant_t* node_id = va_arg(args, variant_t*);

        ZWError err = zway_cc_wakeup_set(zway, record->nodeId, record->instanceId, variant_get_int(interval), variant_get_int(node_id), NULL, NULL, NULL);
        ret_val = variant_create_bool(err == NoError);
    }

    return ret_val;
}

variant_t*   command_class_eval_sensor_multilevel(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    variant_t* param = va_arg(args, variant_t*);
    if(strcmp(method, "Get") == 0)
    {
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        //ZWError err = zway_cc_sensor_multilevel_get(zway, record->nodeId, record->instanceId, -1, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        //ret_val = variant_create_bool(err == NoError);
        char buf[32] = {0};
        snprintf(buf, 31, "%d.val", variant_get_int(param));
        ret_val = command_class_read_data(record, buf);
    }

    return ret_val;
}

variant_t*   command_class_eval_indicator(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;
    variant_t* param = va_arg(args, variant_t*);

    if(strcmp(method, "Get") == 0)
    {
        zway_data_read_ctx_t* ctx = malloc(sizeof(zway_data_read_ctx_t));
        ctx->record = record;
        zway_cc_indicator_get(zway, record->nodeId, record->instanceId, zway_data_read_success_cb, zway_data_read_fail_cb, (void*)ctx);
        // Block here...
        if(0 != event_wait(DataCallback, COMMAND_DATA_READY_EVENT, 1000))
        {
            LOG_DEBUG(DataCallback, "Failed to get a response in 1000 msec");
        }
        ret_val = command_class_read_data(record, NULL);
    }
    else if(strcmp(method, "Set") == 0)
    {
        int arg = variant_get_int(param);
        ZWError err = zway_cc_indicator_set(zway, record->nodeId, record->instanceId, arg, NULL, NULL, NULL);
        ret_val = variant_create_bool(err == NoError);
    }

    return ret_val;
}

variant_t*  command_class_exec(command_class_t* cmd_class, const char* cmd_name, device_record_t* record, ...)
{
    variant_t* retVal = NULL;
    va_list args;
    va_start(args, record);
    retVal = cmd_class->command_impl(cmd_name, record, args);
    va_end(args);

    return retVal;
}
