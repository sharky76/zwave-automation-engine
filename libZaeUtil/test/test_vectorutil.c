#include <stdio.h>
#include "../vector_utils.h"
#include "../variant.h"
int General = 1;

void main()
{
    const char* c = "12.13.14";
    vector_t* new_vec = vector_from_string(c, '.');

    printf("vector count = %d\n", new_vec->count);
    EXPECT_EQ(variant_get_string(vector_pop_front(new_vec)), "12");

    for(int i = 0; i < 3; i++)
    {
        printf("%d: %s\n", i, variant_get_string(vector_pop_front(new_vec)));
    }

    vector_free(new_vec);
}