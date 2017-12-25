#include "stack.h"
#include <stdlib.h>
#include <stdio.h>

variant_stack_t*    stack_create()
{
    variant_stack_t* new_stack = (variant_stack_t*)malloc(sizeof(variant_stack_t));
    new_stack->count = 0;
    new_stack->head = NULL;
    new_stack->tail = NULL;

    pthread_mutex_init(&new_stack->lock, NULL);

    return new_stack;
}

void        stack_free(variant_stack_t* stack)
{
    if(NULL == stack)
    {
        return;
    }

    stack_empty(stack);

    free(stack);
}

void stack_empty(variant_stack_t* stack)
{
    if(NULL == stack)
    {
        return;
    }

    while(stack->count > 0)
    {
        variant_t* v = stack_pop_front(stack);
        variant_free(v);
    }
}

void stack_push_front(variant_stack_t* stack, variant_t* value)
{
    if(NULL != stack)
    {
        stack_item_t* stack_item = (stack_item_t*)calloc(1, sizeof(stack_item_t));
        stack_item->data = value;

        if(stack->count > 0)
        {
            stack->head->prev = stack_item;
            stack_item->next = stack->head;
        }
        else
        {
            stack->tail = stack_item;
        }

        stack_item->prev = NULL;
        stack->head = stack_item;
        stack->count++;
    }
}

variant_t* stack_pop_front(variant_stack_t* stack)
{
    if(NULL != stack && stack->count > 0)
    {
        stack_item_t* head_item = stack->head;
        variant_t* data = head_item->data;

        stack->count--;

        if(stack->count == 0)
        {
            stack->head = stack->tail = NULL;
        }
        else
        {
            stack->head = head_item->next;
            stack->head->prev = NULL;
        }

        free(head_item);


        return data;
    }
    else
    {
        return NULL;
    }
}

void    stack_push_back(variant_stack_t* stack, variant_t* value)
{
    if(NULL != stack)
    {
        stack_item_t* stack_item = (stack_item_t*)malloc(sizeof(stack_item_t));
        stack_item->data = value;
        stack_item->next = NULL;
        stack_item->prev = stack->tail;

        if(stack->count > 0)
        {
            stack->tail->next = stack_item;
        }
        else if(stack->count == 0)
        {
            stack->head = stack_item;
        }
        
        if(stack->count == 1)
        {
            stack->head->next = stack_item;
        }

        stack->tail = stack_item;
        stack->count++;
    }
}

variant_t*   stack_pop_back(variant_stack_t* stack)
{
    if(NULL != stack && stack->count > 0)
    {
        stack_item_t* tail_item = stack->tail;
        variant_t* data = tail_item->data;

        stack->count--;

        if(stack->count == 0)
        {
            stack->head = stack->tail = NULL;
        }
        else
        {
            stack->tail = tail_item->prev;
            stack->tail->next = NULL;
        }

        free(tail_item);
        
        return data;
    }
    else
    {
        return NULL;
    }
}

variant_t*   stack_peek_front(variant_stack_t* stack)
{
    if(stack->count > 0)
    {
        return stack->head->data;
    }
    else
    {
        return NULL;
    }
}

variant_t*      stack_peek_back(variant_stack_t* stack)
{
    if(stack->count > 0)
    {
        return stack->tail->data;
    }
    else
    {
        return NULL;
    }
}

variant_t*   stack_peek_at(variant_stack_t* stack, int index)
{
    if(index >= stack->count)
    {
        return NULL;
    }

    stack_item_t* head = stack->head;

    for(int i = 0; i < index; i++)
    {
        head = head->next;
    }

    return head->data;
}

// --<>---<>---<>---

void stack_remove(variant_stack_t* stack, variant_t* value)
{
    if(stack->count == 0)
    {
        return;
    }

    stack_item_t** head = (&stack->head);
    stack_item_t* tmp = NULL;
    stack_item_t* current = *head;

    if((*head)->data == value)
    {
        current = (*head)->next;
        (*head) = current;
        if(NULL != (*head))
        {
            (*head)->prev = NULL;
        }
    }
    else
    {
        while(current->next != NULL && current->next->data != value)
        {
            current = current->next;
        }
        
        if(NULL == current->next)
        {
            return; // not found
        }
        
        if(current->next->next != NULL)
        {
            tmp = current->next;
            current->next = tmp->next;
            tmp->next->prev = current;
            free(tmp);
        }
        else 
        {
            free(current->next);
            current->next = NULL;
            stack->tail = current;
        }
    }

    stack->count--;
}

variant_stack_t*    stack_sort(variant_stack_t* stack, int (*compare_cb)(const void* left, const void* right))
{
    variant_t** sorted_array = calloc(stack->count, sizeof(variant_t*));
    variant_stack_t* sorted_stack = stack_create();

    int i = 0;
    // Copy stack to array
    stack_for_each(stack, val)
    {
        sorted_array[i++] = val;
    }

    qsort(sorted_array, stack->count, sizeof(sorted_array), compare_cb);

    // Copy sorted array to new stack
    for(int i = 0; i < stack->count; i++)
    {
        stack_push_back(sorted_stack, sorted_array[i]);
    }

    free(sorted_array);
    return sorted_stack;
}

bool            stack_is_exists(variant_stack_t* stack, bool (*match_cb)(variant_t*, void* arg), void* arg)
{
    bool retVal = false;

    stack_for_each(stack, element_var)
    {
        if(match_cb(element_var, arg))
        {
            retVal = true;
            break;
        }
    }

    return retVal;
}

void            stack_lock(variant_stack_t* stack)
{
    pthread_mutex_lock(&stack->lock);
}

void            stack_unlock(variant_stack_t* stack)
{
    pthread_mutex_unlock(&stack->lock);
}

/**
 * Splice the stack starting from Start position and ending at 
 * End (not including). Original stack left intact, new stack is 
 * returned containing elements from start till end-1 
 * 
 * @author alex (12/24/2017)
 * 
 * @param stack 
 * @param start 
 * @param end 
 * 
 * @return variant_stack_t* 
 */
variant_stack_t* stack_splice(variant_stack_t* stack, unsigned int start, unsigned int end)
{
    if(NULL == stack || stack->count == 0 || start >= stack->count || end > stack->count || start > end)
    {
        return NULL;
    }

    variant_stack_t* result_stack = stack_create();

    stack_for_each_range(stack, start, end-1, stack_value)
    {
        stack_push_back(result_stack, stack_value);
    }

    return result_stack;
}


