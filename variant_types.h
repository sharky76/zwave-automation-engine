#pragma once

#include <variant.h>

//extern int DT_SENSOR;
//extern int DT_DEV_RECORD;
//extern int DT_SERVICE;
//extern int DT_DATA_ENTRY_TYPE;
//extern int DT_USER_LOCAL_TYPE;

typedef enum 
{
    T_PARETHESIS = 5000,
    T_OPERATOR,
    T_OPERAND,
    T_FUNCTION,
} TokenType;
