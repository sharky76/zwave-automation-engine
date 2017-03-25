#pragma once

#include "variant.h"
#include <sys/param.h>
// This is the basic stack implementation for holding operands and operators

typedef struct stack_item_t
{
    variant_t* data;
    struct stack_item_t* next;
    struct stack_item_t* prev;
} stack_item_t;

typedef struct variant_stack_t
{
    int count;
    stack_item_t*   head;
    stack_item_t*   tail;
} variant_stack_t;

#define stack_for_each(_stack, _value)  \
int __i__##_value;  \
variant_t* _value = stack_peek_at(_stack, 0);  \
for(__i__##_value = 0; __i__##_value < _stack->count; __i__##_value++, _value = stack_peek_at(_stack, __i__##_value))   \

#define stack_for_each_reverse(_stack, _value)  \
int __i__;  \
variant_t* _value = stack_peek_at(_stack, _stack->count-1);  \
for(__i__ = _stack->count-1; __i__ >= 0; __i__--, _value = stack_peek_at(_stack, __i__))   \

#define stack_for_each_range(_stack, _from, _to, _value)  \
int __i__;  \
variant_t* _value = stack_peek_at(_stack, 0);  \
for(__i__ = _from; __i__ <= MIN(_stack->count-1, _to); __i__++, _value = stack_peek_at(_stack, __i__))   \

variant_stack_t*        stack_create();
void            stack_free(variant_stack_t* stack);
void            stack_empty(variant_stack_t* stack);
void            stack_push_front(variant_stack_t* stack, variant_t* value);
variant_t*      stack_pop_front(variant_stack_t* stack);
variant_t*      stack_peek_front(variant_stack_t* stack);
variant_t*      stack_peek_back(variant_stack_t* stack);
variant_t*      stack_peek_at(variant_stack_t* stack, int index);
void            stack_push_back(variant_stack_t* stack, variant_t* value);
variant_t*      stack_pop_back(variant_stack_t* stack);
void            stack_remove(variant_stack_t* stack, variant_t* value);
variant_stack_t*    stack_sort(variant_stack_t* stack, int (*compare_cb)(const void* left, const void* right));
bool            stack_is_exists(variant_stack_t* stack, bool (*match_cb)(variant_t*, void* arg), void* arg);

