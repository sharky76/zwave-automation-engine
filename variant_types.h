#pragma once

#include <variant.h>

typedef enum 
{
    T_PARETHESIS = 5000,
    T_OPERATOR,
    T_OPERAND,
    T_FUNCTION,
} TokenType;
