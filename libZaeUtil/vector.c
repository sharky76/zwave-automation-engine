#include "vector.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct vector_item_t
{
    variant_t* value;
    vector_item_t* next; 
    vector_item_t* prev;  
} vector_item_t;

vector_t* vector_create()
{
    vector_t* new_vector = (vector_t*)calloc(1, sizeof(vector_t));
    new_vector->count = 0;
    new_vector->first = NULL;
    new_vector->last = NULL;
    pthread_mutex_init(&new_vector->lock, NULL);

    return new_vector;
}

void vector_free(vector_t* vector)
{
    variant_t* value = NULL;

    while((value = vector_pop_front(vector)) != NULL)
    {
        variant_free(value);
    }

    free(vector);
}

void vector_push_back(vector_t* vector, variant_t* value)
{
    vector_item_t* new_item = (vector_item_t*)calloc(1, sizeof(vector_item_t));
    
    new_item->value = value; 
    
    if(0 == vector->count)
    {
        vector->first = vector->last = new_item;
    }
    else
    {
        new_item->prev = vector->last;
        vector->last->next = new_item;
        vector->last = new_item;
    }

    vector->count++;
}

void vector_push_front(vector_t* vector, variant_t* value)
{
    if(0 == vector->count)
    {
        vector_push_back(vector, value);
    }
    else
    {
        vector_item_t* new_item = (vector_item_t*)calloc(1, sizeof(vector_item_t));
        new_item->next = vector->first;
        vector->first->prev = new_item;
        vector->first = new_item;
        new_item->value = value;

        vector->count++;
    }
}

variant_t* vector_pop_front(vector_t* vector)
{
    if(0 == vector->count) return NULL;

    vector_item_t* new_item = vector->first;
    vector->first = new_item->next;
    vector->count--;

    if(0 == vector->count)
    {
        vector->last = vector->first;
    }

    variant_t* value = new_item->value;
    free(new_item);
    return value;
}

variant_t* vector_pop_back(vector_t* vector)
{
    if(0 == vector->count) return NULL;

    vector_item_t* new_item = vector->last;
    vector->last = new_item->prev;
    vector->count--;

    if(0 == vector->count)
    {
        vector->first = vector->last;
    }
    else
    {
        vector->last->next = NULL;
    }

    variant_t* value = new_item->value;
    free(new_item);
    return value;
}

variant_t* vector_peek_front(vector_t* vector)
{
    if(0 == vector->count) return NULL;

    return vector->first->value;
}

variant_t* vector_peek_back(vector_t* vector)
{
    if(0 == vector->count) return NULL;

    return vector->last->value;
}

variant_t* vector_peek_at(vector_t* vector, int index)
{
    if(0 == vector->count) return NULL;

    vector_item_t* item = vector->first;
    int i = 0;
    while(i++ < index)
    {
        item = item->next;
    }

    return item->value;
}

void  vector_remove(vector_t* vector, variant_t* value)
{
    if(0 == vector->count) return;

    vector_item_t* item = vector->first;
    while(item->value != value && item != NULL)
    {
        item = item->next;
    }

    if(NULL != item)
    {
        if(item->next && item->prev)
        {
            item->prev->next = item->next;
            item->next->prev = item->prev;
            variant_free(item->value);
            free(item);
            vector->count--;
        }
        else if(!item->next)
        {
            variant_free(vector_pop_back(vector));
        }
        else if(!item->prev)
        {
            variant_free(vector_pop_front(vector));
        }
    }
}

void  vector_remove_at(vector_t* vector, int index)
{
    if(0 == vector->count) return;

    vector_item_t* item = vector->first;
    int i = 0;
    while(i++ < index)
    {
        item = item->next;
    }

    if(NULL != item)
    {
        if(item->next && item->prev)
        {
            item->prev->next = item->next;
            item->next->prev = item->prev;
            variant_free(item->value);
            free(item);
            vector->count--;
        }
        else if(!item->next)
        {
            variant_free(vector_pop_back(vector));
        }
        else if(!item->prev)
        {
            variant_free(vector_pop_front(vector));
        }
    }
}

bool vector_is_exists(vector_t* vector, bool (*match_cb)(variant_t*, void* arg), void* arg)
{
    if(0 == vector->count) return false;

    vector_item_t* item = vector->first;
    while(item)
    {
        if(match_cb(item->value, arg)) return true;
        item = item->next;
    }

    return false;
}

bool vector_is_empty(vector_t* vector)
{
    return vector->count == 0;
}

void vector_lock(vector_t* vector)
{
    pthread_mutex_lock(&vector->lock);
}

void vector_unlock(vector_t* vector)
{
    pthread_mutex_unlock(&vector->lock);
}

bool vector_trylock(vector_t* vector)
{
    return pthread_mutex_trylock(&vector->lock) == 0;
}