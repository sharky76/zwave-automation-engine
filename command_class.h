#ifndef COMMAND_CLASS_H
#define COMMAND_CLASS_H

// This is the command class table 
#include <ZWayLib.h>
#include "variant.h"

#define MAX_COMMAND_CLASSES     256
#define MAX_COMMAND_NAME_LEN    32

typedef enum CommandMethod
{
    M_GET,
    M_SET,
} CommandMethod;

typedef struct command_class_st
{
    ZWBYTE  command_id;
    const char command_name[MAX_COMMAND_NAME_LEN];

    CommandMethod  supported_method_list[2];

    int  get_args;
    int  set_args;

    variant_t*   (*command_impl)(CommandMethod method, ZWBYTE node_id, ZWBYTE instance_id, va_list);

} command_class_t;

void                init_command_classes();
command_class_t*    get_command_class_by_id(ZWBYTE command_id);
command_class_t*    get_command_class_by_name(const char* command_name);
int                 is_command_valid(const char* command);

variant_t*   command_class_eval_basic(CommandMethod method, ZWBYTE node_id, ZWBYTE instance_id, va_list args);
variant_t*   command_class_eval_binarysensor(CommandMethod method, ZWBYTE node_id, ZWBYTE instance_id, va_list args);

#endif // COMMAND_CLASS_H
