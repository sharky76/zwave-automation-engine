#ifndef COMMAND_CLASS_H
#define COMMAND_CLASS_H

// This is the command class table 
#include <ZWayLib.h>
#include "variant.h"
#include "resolver.h"

#define MAX_COMMAND_CLASSES     256
#define MAX_COMMAND_NAME_LEN    32


typedef struct command_method_t
{
    char* name;
    int   nargs;
    char* help;
} command_method_t;

typedef struct command_class_st
{
    ZWBYTE  command_id;
    const char* command_name;

    command_method_t  supported_method_list[20];

    variant_t*   (*command_impl)(const char*, device_record_t*, va_list);
} command_class_t;

//void                init_command_classes();
command_class_t*    get_command_class_by_id(ZWBYTE command_id);
command_class_t*    get_command_class_by_name(const char* command_name);
void                command_class_for_each(void (*visitor)(command_class_t*, void* arg), void* arg);
variant_t*          command_class_exec(command_class_t* cmd_class, const char* cmd_name, device_record_t* record, ...);
variant_t*          command_class_extract_data(ZDataHolder dh);


#endif // COMMAND_CLASS_H
