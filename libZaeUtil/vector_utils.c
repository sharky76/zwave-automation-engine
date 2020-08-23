#include "vector_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

vector_t* vector_from_string(const char* str, char delim)
{
    vector_t* new_vec = vector_create();
    int len = 0;
    
    char  strcopy[256] = {0};
    strncpy(strcopy, str, 255);
    char* tok = strchr(strcopy, delim);
    variant_t* value;

    while(NULL != tok)
    {
        (*tok) = 0;
        value = variant_create_string(strdup(strcopy+len));
        vector_push_back(new_vec, value);
        *tok = delim;
        len = (tok - strcopy) + 1;
        tok = strchr(strcopy+len, delim);
    }

    value = variant_create_string(strdup(strcopy+len));
    vector_push_back(new_vec, value);
    return new_vec;
}