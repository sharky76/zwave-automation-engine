#ifndef OPERATOR_H
#define OPERATOR_H

#include "variant.h"

/*
    This class defines an operator used in command parser module
*/

typedef enum OperatorType_e
{
    OP_DEVICE_FUNCTION,
    OP_SERVICE_METHOD,
    OP_LEFT_PARETHESIS,
    OP_RIGHT_PARETHESIS,
    OP_UNARY_MINUS,
    OP_MINUS,
    OP_PLUS,
    OP_LESS,
    OP_MORE,
    OP_CMP,
    OP_AND,
    OP_OR,
} OperatorType;

typedef struct operator_t
{
    OperatorType    type;
    void*           operator_data;
    int             (*argument_count)(struct operator_t*);
    variant_t*      (*eval_callback)(struct operator_t*, ...);
} operator_t;

operator_t* operator_create(OperatorType type, void* operator_data);
void        operator_delete_function_operator(void* arg);
void        operator_delete_service_method_operator(void* arg);

#endif // OPERATOR_H
