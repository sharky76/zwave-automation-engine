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
    __asm__ volatile("clz %1, r3"
        : "=r" (bucket)
        : "r" (adjusted_size)
        );
    bucket = sizeof(size)*8 - (bucket+1);
#elif defined ARCH_X86
    __asm__ volatile("bsr %1, %0"
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
    pool->pool = calloc(count, pool->pool_size);

    pool->first = (chunk_descriptor_t*)pool->pool;
    chunk_descriptor_t* chunk = pool->first;

    for(int i = 0; i < count; i++)
    {
        chunk->next = pool->first + pool->chunk_size*i;
        chunk = chunk->next;
        chunk->next = NULL;
    }

    return pool;
}

void*   const_allocator_new(pool_t* pool)
{
    chunk_descriptor_t* victim = pool->first;
    if(NULL != victim)
    {
        pool->free_size -= pool->chunk_size;
        pool->first = victim->next;
        victim->pool = pool;
    }

    return (victim == NULL)? NULL : DATA(victim);
}

void    const_allocator_delete(void* victim)
{
    chunk_descriptor_t* desc = DESCRIPTOR(victim);
    desc->next = desc->pool->first;
    desc->pool->first = desc;
    desc->pool->free_size += desc->pool->chunk_size;
}
