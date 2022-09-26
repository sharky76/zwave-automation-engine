#include <stdlib.h>
#include <stdint.h>

#define MIN_BUCKET 5    // 2^5 = 0x20
#define MAX_BUCKET 31   // 2^31 = 0x80000000
#define NUM_BUCKETS (MAX_BUCKET - MIN_BUCKET)

typedef struct pool_t pool_t;

typedef struct chunk_descriptor_t
{
    uint8_t bucket;
    uint32_t size;
    struct pool_t* pool;
    struct chunk_descriptor_t* next;
} chunk_descriptor_t;

typedef struct pool_t
{
    int chunk_size;
    int free_size;
    int pool_size;
    int count;
    chunk_descriptor_t* first;
    chunk_descriptor_t* buckets[NUM_BUCKETS]; 
    void* pool;
    void* free;
    struct pool_t* next;
} pool_t;

#define DATA(_buffer_) (void*)((void*)_buffer_+sizeof(chunk_descriptor_t))
#define DESCRIPTOR(_buffer_) (chunk_descriptor_t*)((void*)_buffer_-sizeof(chunk_descriptor_t))

pool_t* allocator_init(int size);
void*   allocator_new(pool_t* pool, uint32_t size);
void    allocator_delete(void* victim);
void    allocator_destroy(pool_t* pool);

pool_t* const_allocator_init(int size, int count);
void*   const_allocator_new(pool_t* pool);
void    const_allocator_delete(void* victim);

