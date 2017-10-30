#include "operator.h"
#include "command_class.h"
#include "command_parser.h"
#include "stdarg.h"
#include <service.h>
#include <stdarg.h>
#include <stdio.h>

variant_t*   op_equal(operator_t* op, ...);
variant_t*   op_less(operator_t* op, ...);
variant_t*   op_more(operator_t* op, ...);
variant_t*   op_and(operator_t* op, ...);
variant_t*   op_or(operator_t* op, ...);
variant_t*   op_plus(operator_t* op, ...);
variant_t*   op_minus(operator_t* op, ...);
variant_t*   op_device_function(operator_t* op, ...);
variant_t*   op_service_method(operator_t* op, ...);
variant_t*   op_unary_minus(operator_t* op, ...);

int unary_argument_count(struct operator_t* op);
int binary_argument_count(struct operator_t* op);
int function_argument_count(struct operator_t* op);
int service_method_argument_count(struct operator_t* op);

static operator_t   operator_list[] = {
    {OP_DEVICE_FUNCTION, NULL, &function_argument_count, &op_device_function},
    {OP_SERVICE_METHOD, NULL, &service_method_argument_count, &op_service_method},
    {OP_LEFT_PARETHESIS,    NULL, NULL, NULL},
    {OP_RIGHT_PARETHESIS,   NULL, NULL, NULL},
    {OP_LESS,     NULL, &binary_argument_count, &op_less},
    {OP_MORE,     NULL, &binary_argument_count, &op_more},
    {OP_CMP,      NULL, &binary_argument_count, &op_equal},
    {OP_AND,      NULL, &binary_argument_count, &op_and},
    {OP_OR,       NULL, &binary_argument_count, &op_or},
    {OP_PLUS,     NULL, &binary_argument_count, &op_plus},
    {OP_MINUS,    NULL, &binary_argument_count, &op_minus},
    {OP_UNARY_MINUS, NULL, &unary_argument_count, &op_unary_minus}
};

operator_t* operator_create(OperatorType type, void* operator_data)
{
    operator_t* new_operator = (operator_t*)malloc(sizeof(operator_t));

    operator_t* ref_operator = NULL;

    int i;
    for(i = 0; i < sizeof(operator_list) / sizeof(operator_t) && ref_operator == NULL; i++)
    {
        if(operator_list[i].type == type)
        {
            ref_operator = &operator_list[i];
        }
    }

    if(NULL != ref_operator)
    {
        new_operator->type = type;
        new_operator->operator_data = operator_data;
        new_operator->argument_count = ref_operator->argument_count;
        new_operator->eval_callback = ref_operator->eval_callback;
        return new_operator;
    }
    else
    {
        return NULL;
    }
}

variant_t*   op_equal(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg2 = va_arg(args, variant_t*);

    return variant_create_bool(variant_compare(arg1, arg2) == 0);
}

variant_t*   op_less(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg2 = va_arg(args, variant_t*);

    return variant_create_bool((variant_compare(arg1, arg2) < 0));
}

variant_t*   op_more(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg2 = va_arg(args, variant_t*);

    return variant_create_bool(variant_compare(arg1, arg2) > 0);
}

variant_t*   op_and(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg2 = va_arg(args, variant_t*);

    return variant_create_bool(variant_get_bool(arg1) && variant_get_bool(arg2));
}

variant_t*   op_or(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg2 = va_arg(args, variant_t*);

    return variant_create_bool(variant_get_bool(arg1) || variant_get_bool(arg2));
}

variant_t*   op_plus(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg2 = va_arg(args, variant_t*);

    return variant_add(arg1, arg2);
}

variant_t*   op_minus(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg2 = va_arg(args, variant_t*);

    return variant_subtract(arg1, arg2);
}

variant_t*   op_unary_minus(operator_t* op, ...)
{
    va_list args;
    va_start(args, op);

    variant_t* arg1 = va_arg(args, variant_t*);
    variant_t* arg0 = variant_create_int32(DT_INT32, 0);
    variant_t* result = variant_subtract(arg0, arg1);
    variant_free(arg0);
    return result;
}

variant_t*   op_device_function(operator_t* op, ...)
{
    function_operator_data_t* op_data = (function_operator_data_t*)op->operator_data;
    //return op_data->command_class->command_impl(op_data->command_method, op_data->instance_id, );
    va_list args;
    va_start(args, op);

    if(NULL != op_data->command_class)
    {
        return op_data->command_class->command_impl((const char*)op_data->command_method, op_data->device_record, args);
    }
    else
    {
        return NULL;
    }
}

variant_t*   op_service_method(operator_t* op, ...)
{
    service_method_t* method = (service_method_t*)op->operator_data;

    va_list args;
    va_start(args, op);

    return method->eval_callback(method, args);
}

int unary_argument_count(struct operator_t* op)
{
    return 1;
}

int binary_argument_count(struct operator_t* op)
{
    return 2;
}

int function_argument_count(struct operator_t* op)
{
    function_operator_data_t* op_data = (function_operator_data_t*)op->operator_data;
    
    if(NULL == op_data->command_class)
    {
        return 0;
    }

    command_method_t* supported_methods = op_data->command_class->supported_method_list;

    while(supported_methods->name)
    {
        if(strcmp(supported_methods->name, op_data->command_method) == 0)
        {
            return supported_methods->nargs;
        }
        supported_methods++;
    }
    return 0;
}

int service_method_argument_count(struct operator_t* op)
{
    service_method_t* method = (service_method_t*)op->operator_data;
    return method->args;
}

void operator_delete_function_operator(void* arg)
{
    operator_t* op = (operator_t*)arg;
    free(op->operator_data);
    free(op);
}

void operator_delete_service_method_operator(void* arg)
{
    operator_t* op = (operator_t*)arg;
    free(op);
}

