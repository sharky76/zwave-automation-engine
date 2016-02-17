#include "variant.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int  auto_inc_data_type = DT_USER+1;

void    variant_delete_default(void* ptr)
{
    free(ptr);
}

void    variant_delete_none(void* ptr)
{
    // do nothing
}

void    variant_delete_variant(void* ptr)
{
    variant_t* v = (variant_t*)ptr;
    variant_free(v);
}

bool variant_compare_bool(const variant_t* v, const variant_t* other)
{
    return v->storage.bool_data == other->storage.bool_data;
}

bool variant_compare_float(const variant_t* v, const variant_t* other)
{
    return v->storage.double_data == other->storage.double_data;
}

bool variant_compare_int(const variant_t* v, const variant_t* other)
{
    return v->storage.int_data == other->storage.int_data;
}

variant_t*  variant_create(VariantDataType type, void* data)
{
    variant_t* new_variant = (variant_t*)calloc(1, sizeof(variant_t));
    new_variant->type = type;
    new_variant->delete_cb = NULL;
    new_variant->compare_cb = NULL;
    new_variant->ref_count = 1;

    switch(type)
    {
    case DT_BOOL:
        new_variant->storage.bool_data = (bool)data;
        new_variant->compare_cb = &variant_compare_bool;
        break;
    case DT_FLOAT:
        new_variant->storage.double_data = *(double*)data;
        new_variant->compare_cb = &variant_compare_float;
        break;
    case DT_INT32:
        new_variant->storage.int_data = (int32_t)data;
        break;
    case DT_PTR:
    case DT_STRING:
    default:
        new_variant->storage.ptr_data = data;
        new_variant->delete_cb = &variant_delete_default;
        break;
    }

    return new_variant;
}

variant_t*  variant_create_int32(int type, int data)
{
    variant_t* ret = variant_create(DT_INT32, (void*)data);
    ret->type = type;
    return ret;
}

variant_t*  variant_create_bool(bool data)
{
    variant_t* new_variant = variant_create(DT_BOOL, (void*)data); //(variant_t*)malloc(sizeof(variant_t));
    return new_variant;
}

variant_t*  variant_create_ptr(int type, void* ptr, void (*delete_cb)(void* arg))
{
    variant_t* ret = variant_create(DT_PTR, ptr);
    ret->type = type;

    if(NULL != delete_cb)
    {
        ret->delete_cb = delete_cb;
    }
    else
    {
        ret->delete_cb = &variant_delete_default;
    }
    return ret;
}

variant_t*  variant_create_variant(int type, variant_t* variant)
{
    variant_t* ret = variant_create(DT_PTR, variant);
    ret->type = type;
    ret->delete_cb = &variant_delete_variant;
    return ret;
}

variant_t*  variant_create_float(double data)
{
    variant_t* new_variant = variant_create(DT_FLOAT, NULL);
    new_variant->storage.double_data = data;

    return new_variant;
}

variant_t*  variant_create_string(const char* string)
{
    return variant_create(DT_STRING, (void*)string);
}

void        variant_free(variant_t* variant)
{
    if(--variant->ref_count == 0)
    {
        if(NULL != variant->delete_cb)
        {
            variant->delete_cb(variant->storage.ptr_data);
        }
    
        free(variant);
    }
}

bool         variant_get_bool(variant_t* variant)
{
    return variant->storage.bool_data;
}

int         variant_get_int(variant_t* variant)
{
    return (int)variant->storage.int_data;
}

float       variant_get_float(variant_t* variant)
{
    return variant->storage.double_data;
}

const char* variant_get_string(variant_t* variant)
{
    return (const char*)variant->storage.ptr_data;
}

void*       variant_get_ptr(variant_t* variant)
{
    return variant->storage.ptr_data;
}

variant_t*  variant_get_variant(variant_t* variant)
{
    variant_t* ret = (variant_t*)variant->storage.ptr_data;
    ret->ref_count++;

    return ret;
}

int         variant_equal(variant_t* v1, variant_t* v2)
{
    if(NULL != v1->compare_cb)
    {
        return v1->compare_cb(v1, v2);
    }

    if(v1->type == DT_STRING && v2->type == DT_STRING)
    {
        return !strcmp(variant_get_string(v1), variant_get_string(v2));
    }

    return (v1->storage.ptr_data == v2->storage.ptr_data);
}

bool         variant_less_than(variant_t* v1, variant_t* v2)
{
    if(v1->type != v2->type)
    {
        return v1->type < v2->type;
    }

    if(v1->type == DT_STRING && v2->type == DT_STRING)
    {
        return (strcmp(variant_get_string(v1), variant_get_string(v2)) < 0)? true : false;
    }

    return v1->storage.double_data < v2->storage.double_data;
}

int         variant_more_than(variant_t* v1, variant_t* v2)
{
    return !variant_less_than(v1, v2);
}

int         variant_compare(variant_t* v1, variant_t* v2)
{
    if(!variant_equal(v1,v2))
    {
        if(!variant_less_than(v1,v2))
        {
            return 1;
        }

        return -1;
    }

    return 0;
}

bool        variant_is_null(variant_t* variant)
{
    return variant->storage.ptr_data == 0;
}

bool variant_to_string(variant_t* variant, char** string)
{
    bool retVal = true;
    switch(variant->type)
    {
    case DT_STRING:
        *string = strdup(variant_get_string(variant));
        break;
    case DT_INT8:
    case DT_INT32:
    case DT_INT64:
        {
            *string = (char*)calloc(12, sizeof(char));
            int n = snprintf(*string, 1, "%d", variant_get_int(variant));
            retVal = (n > 0);
        }
        break;
    case DT_BOOL:
        {
            *string = (char*)calloc(6, sizeof(char));
            if(variant_get_bool(variant))
            {
                strcat(*string, "True");
            }
            else
            {
                strcat(*string, "False");
            }
        }
        break;
    default:
        retVal = false;
    }

    return retVal;
}

int variant_get_next_user_type()
{
    return auto_inc_data_type++;
}
