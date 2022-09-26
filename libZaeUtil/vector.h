#pragma once 

#include <sys/param.h>
#include <pthread.h>
#include "variant.h"
#include "allocator.h"

typedef struct vector_item_t vector_item_t;

typedef struct vector_t
{
    vector_item_t* first;
    vector_item_t* last;
    int count;
    pool_t* pool;
    pthread_mutex_t lock;
} vector_t;

vector_t* vector_create();
void vector_free(vector_t* vector);
void vector_push_back(vector_t* vector, variant_t* value);
void vector_push_front(vector_t* vector, variant_t* value);
variant_t* vector_pop_front(vector_t* vector);
variant_t* vector_pop_back(vector_t* vector);

variant_t* vector_peek_front(vector_t* vector);
variant_t* vector_peek_back(vector_t* vector);
variant_t* vector_peek_at(vector_t* vector, int index);

void       vector_remove(vector_t* vector, variant_t* value);
void       vector_remove_at(vector_t* vector, int index);

bool       vector_is_exists(vector_t* vector, bool (*match_cb)(variant_t*, void* arg), void* arg);
bool       vector_is_empty(vector_t* vector);
variant_t* vector_find(vector_t* vector, bool (*match_cb)(variant_t*, void* arg), void* arg);

void       vector_lock(vector_t* vector);
void       vector_unlock(vector_t* vector);
bool       vector_trylock(vector_t* vector);      

void vector_for_each(vector_t* vector, void (*visitor)(variant_t*, void*), void* arg);