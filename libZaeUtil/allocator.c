#include "allocator.h"
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define ARCH_X86 
#else
#define ARCH_ARM
#endif

pool_t* allocator_init(int size)
{
    pool_t* pool = calloc(1, sizeof(pool_t));
    pool->pool_size = pool->free_size = size;
    pool->pool = calloc(1, size);
    pool->free = pool->pool;
}

void*   allocator_new(pool_t* pool, uint32_t size)
{
    uint32_t bucket;
    uint32_t adjusted_size = size + sizeof(chunk_descriptor_t);

#if defined ARCH_ARM
    asm volatile("clz %1, r3"
        : "=r" (bucket)
        : "r" (adjusted_size)
        );
    bucket = sizeof(size)*8 - (bucket+1);
#elif defined ARCH_X86
    asm volatile("bsr %1, %0"
        : "=r" (bucket)
        : "r" (adjusted_size)
        );
#endif

    int alloc = 0;

    if(bucket < MIN_BUCKET) 
    {
        bucket = MIN_BUCKET-1;
        alloc = 1 << MIN_BUCKET;
    }
    else if(bucket <= MAX_BUCKET)
    {
        alloc = 1 << (bucket+1);
    }

    bucket -= (MIN_BUCKET-1);

    pool_t* work_pool = pool;
    while(work_pool)
    {    
        if(alloc > work_pool->free_size && work_pool->buckets[bucket] == NULL) 
        {
            work_pool = work_pool->next;
        }
        else
        {
            break;
        }
    }

    if(work_pool == NULL)
    {
        pool_t* new_pool = allocator_init(pool->pool_size);
        new_pool->next = pool->next;
        pool->next = new_pool;
        work_pool = new_pool;
    }

    if(work_pool->buckets[bucket] != NULL)
    {
        chunk_descriptor_t* chunk = work_pool->buckets[bucket];
        work_pool->buckets[bucket] = chunk->next;
        return DATA(chunk);
    }
    else
    {
        chunk_descriptor_t* chunk = (chunk_descriptor_t*)work_pool->free;
        chunk->pool = work_pool;
        chunk->bucket = bucket;
        chunk->size = adjusted_size;

        work_pool->free = (void*)work_pool->free + alloc;
        chunk->next = work_pool->free;

        work_pool->free_size -= alloc;
        return DATA(chunk);
    }
}

void    allocator_delete(void* victim)
{
    if(victim == NULL) return;

    chunk_descriptor_t* chunk = DESCRIPTOR(victim);

    // Return chunk directly to its bucket
    pool_t* pool = chunk->pool;
    size_t alloc = 1 << (chunk->bucket + MIN_BUCKET);
    
    // Simple case - if we allocate and de-allocate without any allocation in between
    if(chunk->next == pool->free)
    {
        pool->free = (void*)pool->free - alloc;
    }
    else
    {
        chunk_descriptor_t* first = pool->buckets[chunk->bucket];
        pool->buckets[chunk->bucket] = chunk;
        chunk->next = first;
    }
    pool->free_size += alloc;
}

void    allocator_destroy(pool_t* pool)
{
    while(pool->next)
    {
        allocator_destroy(pool->next);
        break;
    }
    free(pool->pool);
    free(pool);
}

pool_t* const_allocator_init(int size, int count)
{
    pool_t* pool = calloc(1, sizeof(pool_t));
    pool->chunk_size = size + sizeof(chunk_descriptor_t);
    pool->pool_size = pool->free_size = pool->chunk_size * count;
    pool->count = count;
    pool->pool = calloc(count, pool->pool_size);
    pool->free = pool->pool;
}

void*   const_allocator_new(pool_t* pool)
{
    chunk_descriptor_t* chunk = NULL;
    pool_t* work_pool = pool;

    while(work_pool)
    {    
        if(work_pool->chunk_size > work_pool->free_size && work_pool->first == NULL) 
        {
            work_pool = work_pool->next;
        }
        else
        {
            break;
        }
    }

    if(work_pool == NULL)
    {
        pool_t* new_pool = const_allocator_init(pool->chunk_size - sizeof(chunk_descriptor_t), pool->count);
        new_pool->next = pool->next;
        pool->next = new_pool;
        work_pool = new_pool;
    }

    // We will land here with either new pool or pool with available chunk
    if(work_pool->first != NULL)
    {
        chunk = (chunk_descriptor_t*)work_pool->first;
        work_pool->first = chunk->next;
    }
    else
    {
        chunk = (chunk_descriptor_t*)work_pool->free;
        chunk->pool = work_pool;
        work_pool->free = (void*)work_pool->free + work_pool->chunk_size;
    }

    work_pool->free_size -= work_pool->chunk_size;

    return DATA(chunk);
}

void    const_allocator_delete(void* victim)
{
    chunk_descriptor_t* chunk = DESCRIPTOR(victim);

    // Return chunk directly to its bucket
    pool_t* pool = chunk->pool;
    chunk->next = pool->first;
    pool->first = chunk;
    pool->free_size += pool->chunk_size;
}
