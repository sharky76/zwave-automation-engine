#pragma once

#include <variant.h>

typedef enum 
{
    DT_DEV_EVENT_DATA = DT_USER+1,
    DT_DEV_RECORD,
    DT_SERVICE,
    DT_DATA_ENTRY_TYPE,
    DT_USER_LOCAL_TYPE
} LocalVariantDataType;

typedef enum 
{
    T_PARETHESIS = DT_USER_LOCAL_TYPE+1,
    T_OPERATOR,
    T_OPERAND,
    T_FUNCTION,
} TokenType;
