#pragma once

/* This is the parser for processing logic expressions
  used in scene definitions

  Example:

    Patio Door Sensor.0.SensorBinary.Get(1.level) == True
    Room Lights.0.Basic.Set(On)
    Motion Detector.0.Basic.Set(On)
    Motion Detector.0.Basic.Get() == On & Time.0.Basic.Get() > 20:00"
    Timer.0.Basic.Set(10, ConditionalLightsOff)

  Syntax:

    Get Path: 
        [Device name].[Instance].[CommandClass].Get(data)

    Operators:
        ==,&&,||,>, <, (, )

    Set Path:
        [Device name].[Instance].[CommandClass].Set(arguments)

    Reserved words:
        True, False, On, Off

    Each device knows its command classes and provides callbacks for Set and Get with relevant argument requirements
    Operator precedence is same as in C
*/

#include "command_class.h"
#include "resolver.h"
#include "stack.h"
#include "operator.h"
#include "variant.h"
#include "variant_types.h"


#define MAX_COMMAND_ARGUMENTS   3
#define MAX_METHOD_LEN          32

/*typedef enum OperandType_e
{
    DEVICE,
    RESERVED_WORD
} OperandType;*/




typedef struct function_operator_data_st
{
    device_record_t*  device_record;
    //ZWBYTE            instance_id;
    command_class_t*  command_class;
    char              command_method[MAX_METHOD_LEN];
} function_operator_data_t;

typedef struct service_method_operator_data_t
{
    
} service_method_operator_data_t;

typedef struct reserved_word_st
{
    char  word[10];
    int   value;
} reserved_word_t;

variant_stack_t*    command_parser_compile_expression(const char* expression, bool* isOk);
variant_t*          command_parser_execute_expression(variant_stack_t* compiled_expression);
void                command_parser_print_stack(variant_stack_t* compiled_expression);

