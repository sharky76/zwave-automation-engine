#include "command_class.h"
#include "data_callbacks.h"

extern ZWay zway;

static command_class_t command_class_table[] = {
    {0x20, "Basic",        {M_GET, M_SET}, 1,   1,  &command_class_eval_basic},
    {0x30, "SensorBinary", {M_GET},        1,   0,  &command_class_eval_binarysensor}

    /* other standard command classes */
};

static char* command_list[5] = {"Set", "Get", 0};

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

int is_command_valid(const char* command)
{
    int i = 0;
    int retVal = FALSE;

    for(i = 0; i < sizeof(command_list) / sizeof(char*); i++)
    {
        if(strcmp(command_list[i], command) == 0)
        {
            retVal = TRUE;
        }
    }

    return retVal;
}

variant_t*   command_class_eval_basic(CommandMethod method, ZWBYTE node_id, ZWBYTE instance_id, va_list args)
{

    variant_t* arg1 = va_arg(args, variant_t*);

    printf("Arg: %s\n", variant_get_string(arg1));

    switch(method)
    {
    case M_GET:
        zway_cc_basic_get(zway, node_id, instance_id, NULL, NULL, NULL);
        break;
    case M_SET:
        break;
    default:
        break;
    }

    return variant_create(DT_BOOL, (void*)0);
}

variant_t*   command_class_eval_binarysensor(CommandMethod method, ZWBYTE node_id, ZWBYTE instance_id, va_list args)
{
    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* ret_val = NULL;

    switch(method)
    {
    case M_GET:
    {
        zdata_acquire_lock(ZDataRoot(zway));
        ZDataHolder dh = zway_find_device_instance_cc_data(zway, node_id, instance_id, 0x30, variant_get_string(arg1));
        ZWBOOL bool_val;
        zdata_get_boolean(dh, &bool_val);
        ret_val = variant_create_bool((bool)bool_val);
        //printf("Found data for device %s: %d\n", event_data->device_name, bool_val);
        zdata_release_lock(ZDataRoot(zway));
    }
        break;
    case M_SET:
        break;
    default:
        break;
    }

    return ret_val;
}
