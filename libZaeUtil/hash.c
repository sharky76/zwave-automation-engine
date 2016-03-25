#include "hash.h"
#include "logger.h"
#include <stdlib.h>


/*********************************************** 
 
    Primes computation...
 
************************************************/
static const size_t small_primes[] =
{
    2,
    3,
    5,
    7,
    11,
    13,
    17,
    19,
    23,
    29
};

static const size_t indices[] =
{
    1,
    7,
    11,
    13,
    17,
    19,
    23,
    29
};

bool
is_prime(size_t x)
{
    const size_t N = sizeof(small_primes) / sizeof(small_primes[0]);
    for (size_t i = 3; i < N; ++i)
    {
        const size_t p = small_primes[i];
        const size_t q = x / p;
        if (q < p)
            return true;
        if (x == q * p)
            return false;
    }
    for (size_t i = 31; true;)
    {
        size_t q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 6;

        q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 4;

        q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 2;

        q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 4;

        q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 2;

        q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 4;

        q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 6;

        q = x / i;
        if (q < i)
            return true;
        if (x == q * i)
            return false;
        i += 2;
    }
    return true;
}

size_t next_prime(size_t n)
{
    const size_t superN = n;
    const size_t L = 30;
    const size_t N = sizeof(small_primes) / sizeof(small_primes[0]);
    // If n is small enough, search in small_primes
    if (n <= small_primes[N-1])
    {
        for(int i = 0; i < N; i++)
        {
            if(small_primes[i] > n)
            {
                return small_primes[i];
            }
        }
    }
    // Else n > largest small_primes
    // Start searching list of potential primes: L * k0 + indices[in]
    const size_t M = sizeof(indices) / sizeof(indices[0]);
    // Select first potential prime >= n
    //   Known a-priori n >= L
    size_t k0 = n / L;
    size_t in = 0;//std::lower_bound(indices, indices + M, n - k0 * L) - indices;

    for(int i = 0; i < M; i++)
    {
        if(indices[i] > n - k0 * L)
        {
            in = i;
            break;
        }
    }

    n = L * k0 + indices[in];
    while (!is_prime(n) || n <= superN)
    {
        if (++in == M)
        {
            ++k0;
            in = 0;
        }
        n = L * k0 + indices[in];
    }
    return n;
}
/********************************************************************************************/



static unsigned int DEFAULT_HASH_SIZE = 7; //89; //5003;

void delete_node_data(hash_node_data_t* node_data)
{
    if(NULL != node_data)
    {
        variant_free(node_data->data);
        free(node_data);
    }
}

hash_node_data_t*   create_node_data(uint32_t key, variant_t* data)
{
    hash_node_data_t* new_data = calloc(1, sizeof(hash_node_data_t));
    new_data->data = data;
    new_data->key = key;

    return new_data;
}

typedef struct
{
    int hash_size;
    hash_node_data_t** node_array;
    bool    is_resize_needed;
} copy_items_data_t;

void    copy_hash_items(hash_node_data_t* data, void* arg)
{
    copy_items_data_t* copy_data = (copy_items_data_t*)arg;
    hash_node_data_t** node_array = copy_data->node_array;

    if(copy_data->is_resize_needed)
    {
        return;
    }

    int hash_index = data->key % copy_data->hash_size;

    //printf("Copy hash new index: %u, key %u, data %s\n", hash_index, data->key, variant_get_string(data->data));

    if(node_array[hash_index] != NULL)
    {
        copy_data->is_resize_needed = true;
    }
    else
    {
        node_array[hash_index] = data;
    }
}

hash_table_t*   variant_hash_init()
{
    hash_table_t* hash_table = calloc(1, sizeof(hash_table_t));
    hash_table->node_array = calloc(DEFAULT_HASH_SIZE, sizeof(hash_node_data_t*));
    hash_table->count = 0;
    hash_table->hash_size = DEFAULT_HASH_SIZE;

    return hash_table;
}

void            variant_hash_free(hash_table_t* hash_table)
{
    for(int i = 0; i < hash_table->hash_size; i++)
    {
        //stack_free(hash_table->node_array[i]->data_list);
        //free(hash_table->node_array[i]);
        delete_node_data(hash_table->node_array[i]);
    }

    free(hash_table->node_array);
    //stack_free(hash_table->hash_items);
    free(hash_table);
}

void            variant_hash_free_void(void* hash_table)
{
    variant_hash_free((hash_table_t*)hash_table);
}

void            variant_hash_insert_data(hash_table_t* hash_table, hash_node_data_t* hash_item)
{
    int hash_index = hash_item->key % hash_table->hash_size;

    if(NULL != hash_table->node_array[hash_index])
    {
        if(hash_table->node_array[hash_index]->key == hash_item->key)
        {
            variant_hash_remove(hash_table, hash_item->key);
            hash_table->node_array[hash_index] = hash_item;
            hash_table->count++;
        }
        else
        {
            bool resize_needed = true;
            hash_node_data_t** new_node_array;
            int new_hash_size = next_prime(hash_table->hash_size*2);

            while(resize_needed)
            {
                // Collision detected... Hash table must grow!
                //printf("Collision detected in %p, new size is %d\n", hash_table, new_hash_size);
                new_node_array = calloc(new_hash_size, sizeof(hash_node_data_t*));
    
                copy_items_data_t data;
                data.hash_size = new_hash_size;
                data.node_array = new_node_array;
                data.is_resize_needed = false;
                variant_hash_for_each(hash_table, copy_hash_items, &data);
        
                if(data.is_resize_needed)
                {
                    free(new_node_array);
                    //printf("More resize needed\n");
                    new_hash_size = next_prime(new_hash_size);
                }
                else
                {
                    resize_needed = false;
                }
            }

            // Delete all node_array
            free(hash_table->node_array);
    
            // Set new array
            hash_table->node_array = new_node_array;
            hash_table->hash_size = new_hash_size;

            variant_hash_insert_data(hash_table, hash_item);
        }
    }
    else
    {
        hash_table->node_array[hash_index] = hash_item;
        hash_table->count++;
    }
}

void            variant_hash_insert(hash_table_t* hash_table, uint32_t key, variant_t* item)
{
    hash_node_data_t* hash_item = create_node_data(key, item);
    variant_hash_insert_data(hash_table, hash_item);
}

variant_t*      variant_hash_get(hash_table_t* hash_table, uint32_t key)
{
    int hash_index = key % hash_table->hash_size;

    //printf("Get hash %p key = %u, index = %u\n", hash_table, key, hash_index);
    if(NULL != hash_table->node_array[hash_index] && hash_table->node_array[hash_index]->key == key)
    {
        return hash_table->node_array[hash_index]->data;
    }

    return NULL;
}

void            variant_hash_remove(hash_table_t* hash_table, uint32_t key)
{
    int hash_index = key % hash_table->hash_size;

    if(NULL != hash_table->node_array[hash_index])
    {
        delete_node_data(hash_table->node_array[hash_index]);
        hash_table->node_array[hash_index] = NULL;
        hash_table->count--;
    }
}

void            variant_hash_for_each(hash_table_t* hash_table, void (*visitor)(hash_node_data_t*, void*), void* arg)
{
    int counter = hash_table->count;
    for(int i = 0; i < hash_table->hash_size && counter > 0; i++)
    {
        if(NULL != hash_table->node_array[i])
        {
            visitor(hash_table->node_array[i], arg);
            counter--;
        }
    }
}

/*void            variant_hash_for_each_value(hash_table_t* hash_table, void (*visitor)(variant_t*, void*), void* arg)
{
    int counter = hash_table->count;
    for(int i = 0; i < hash_table->hash_size && counter > 0; i++)
    {
        if(NULL != hash_table->node_array[i])
        {
            visitor(hash_table->node_array[i]->data, arg);
            counter--;
        }
    }
}*/

void    print_string(hash_node_data_t* node_data, void* arg)
{
    printf("Key: %u value: %s\n", node_data->key, variant_get_string(node_data->data));
}

void            variant_hash_print(hash_table_t* hash_table)
{
    variant_hash_for_each(hash_table, print_string, NULL);
}


hash_iterator_t*    variant_hash_begin(hash_table_t* hash_table)
{
    hash_iterator_t* begin = malloc(sizeof(hash_iterator_t));
    begin->hash_table = hash_table;
    begin->index = -1;

    return begin;
}

hash_iterator_t*    variant_hash_end(hash_table_t* hash_table)
{
    hash_iterator_t* end = malloc(sizeof(hash_iterator_t));
    end->hash_table = hash_table;
    end->index = hash_table->hash_size+1;

    return end;

}

bool                variant_hash_iterator_is_end(hash_iterator_t* it)
{
    bool retVal = false;
    hash_iterator_t* end = variant_hash_end(it->hash_table);

    retVal = end->index == it->index;
    free(end);
    return retVal;
}


hash_iterator_t*    variant_hash_iterator_next(hash_iterator_t* it)
{
    for(int i = it->index+1; i < it->hash_table->hash_size; i++)
    {
        if(it->hash_table->node_array[i] != NULL)
        {
            it->index = i;
            return it;
        }
    }

    // Value not found, return end iterator index
    it->index = it->hash_table->hash_size+1;
    return it;
}

variant_t*          variant_hash_iterator_value(hash_iterator_t* it)
{
    return it->hash_table->node_array[it->index]->data;
}
