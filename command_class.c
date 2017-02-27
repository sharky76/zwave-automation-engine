#include "command_class.h"
#include "data_callbacks.h"
#include <logger.h>

extern ZWay zway;

variant_t*   command_class_eval_basic(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_binarysensor(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_battery(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_alarm(const char* method, device_record_t* record, va_list args);
variant_t*   command_class_eval_binaryswitch(const char* method, device_record_t* record, va_list args);

static command_class_t command_class_table[] = {
    {0x20, "Basic",        {"Get", 0, "level", "Set", 1, "level", NULL, 0, NULL},       &command_class_eval_basic},
    {0x30, "SensorBinary", {"Get", 1, "1.level", NULL, 0, NULL},                        &command_class_eval_binarysensor},
    {0x80, "Battery",      {"Get", 1, "last", NULL, 0, NULL},                           &command_class_eval_battery},
    {0x71, "Alarm",        {"Get", 1, "<type>.<field>",  "Set", 2, "<type>, <level>"},  &command_class_eval_alarm},
    {0x25, "SwitchBinary", {"Get", 1, "1.level",  "Set", 1, "True/False"},              &command_class_eval_binaryswitch},


    /* other standard command classes */
    {0, NULL,   {NULL, 0, NULL},   NULL}
};

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
        zway_cc_basic_get(zway, record->nodeId, record->instanceId, NULL, NULL, NULL);
        zdata_acquire_lock(ZDataRoot(zway));
        ZDataHolder dh = zway_find_device_instance_data(zway, record->nodeId, record->instanceId, "level");
        int int_val;
        zdata_get_integer(dh, &int_val);
        ret_val = variant_create_int32(DT_INT32, int_val);
        zdata_release_lock(ZDataRoot(zway));
    }
    else if(strcmp(method, "Set") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);
        zway_cc_basic_set(zway, record->nodeId, record->instanceId, variant_get_byte(arg1), NULL, NULL, NULL);
    }

    return ret_val;
}

variant_t*   command_class_eval_binarysensor(const char* method, device_record_t* record, va_list args)
{
    
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);
        /*zdata_acquire_lock(ZDataRoot(zway));
        ZDataHolder dh = zway_find_device_instance_cc_data(zway, record->nodeId, record->instanceId, record->commandId, variant_get_string(arg1));
        ZWBOOL bool_val;
        zdata_get_boolean(dh, &bool_val);
        ret_val = variant_create_bool((bool)bool_val);
        zdata_release_lock(ZDataRoot(zway));
        */
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
        /*zdata_acquire_lock(ZDataRoot(zway));
        ZDataHolder dh = zway_find_device_instance_cc_data(zway, record->nodeId, record->instanceId, record->commandId, "last");
        int int_val;
        zdata_get_integer(dh, &int_val);
        ret_val = variant_create_int32(DT_INT32, int_val);
        zdata_release_lock(ZDataRoot(zway));
        */
        ret_val = command_class_read_data(record, variant_get_string(arg1));
    }

    return ret_val;
}

variant_t*   command_class_eval_alarm(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;

    if(strcmp(method, "Get") == 0)
    {
        variant_t* arg1 = va_arg(args, variant_t*);
        //printf("eval alarm with arg: %s\n", variant_get_string(arg1));
        ret_val = command_class_read_data(record, variant_get_string(arg1));
    }
    else if(strcmp(method, "Set") == 0)
    {
        variant_t* alarm_type = va_arg(args, variant_t*);
        variant_t* level = va_arg(args, variant_t*);
        zway_cc_alarm_set(zway,record->nodeId, record->instanceId, variant_get_int(alarm_type), variant_get_int(level), NULL, NULL, NULL);
    }

    return ret_val;
}

variant_t*   command_class_eval_binaryswitch(const char* method, device_record_t* record, va_list args)
{
    variant_t* ret_val = NULL;
    variant_t* arg1 = va_arg(args, variant_t*);

    if(strcmp(method, "Get") == 0)
    {
        ret_val = command_class_read_data(record, variant_get_string(arg1));
    }
    else if(strcmp(method, "Set") == 0)
    {
        zway_cc_switch_binary_set(zway,record->nodeId, record->instanceId, variant_get_bool(arg1), NULL, NULL, NULL);
    }

    return ret_val;
}

