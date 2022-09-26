#include "../vector.h"
#include <stdio.h>

int General = 1;

void main()
{
    vector_t* vec = vector_create();

    for(int i = 0; i < 100; i++)
    {
        variant_t* value = variant_create_int32(DT_INT8, i);
        vector_push_back(vec, value);
    }

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 105; i++)
    {
        variant_t* value = vector_pop_back(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);

    for(int i = 0; i < 100; i++)
    {
        variant_t* value = variant_create_int32(DT_INT8, i);
        vector_push_front(vec, value);
    }

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 105; i++)
    {
        variant_t* value = vector_pop_front(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);



    for(int i = 0; i < 100; i++)
    {
        variant_t* value = variant_create_int32(DT_INT8, i);
        vector_push_front(vec, value);
    }

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 105; i++)
    {
        variant_t* value = vector_pop_back(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);


    for(int i = 0; i < 100; i++)
    {
        variant_t* value = variant_create_int32(DT_INT8, i);
        vector_push_back(vec, value);
    }

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 105; i++)
    {
        variant_t* value = vector_pop_front(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);

    {
    variant_t* value = variant_create_int32(DT_INT8, 0);
    vector_push_back(vec, value);

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 5; i++)
    {
        variant_t* value = vector_pop_front(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);
    }

    {
    variant_t* value = variant_create_int32(DT_INT8, 0);
    vector_push_back(vec, value);

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 5; i++)
    {
        variant_t* value = vector_pop_back(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);
    }

    {
    variant_t* value = variant_create_int32(DT_INT8, 0);
    vector_push_front(vec, value);

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 5; i++)
    {
        variant_t* value = vector_pop_front(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);
    }

    {
    variant_t* value = variant_create_int32(DT_INT8, 0);
    vector_push_front(vec, value);

    printf("1 vec size: %d\n", vec->count);

    for(int i = 0; i < 5; i++)
    {
        variant_t* value = vector_pop_back(vec);
        if(NULL != value)
        {
            printf("Val: %d\n", variant_get_int(value));
            variant_free(value);
        }
        else
        {
            printf("%d: NULL value\n", i);
        }
        
    }
    printf("2 vec size: %d\n", vec->count);
    }

    vector_free(vec);
}