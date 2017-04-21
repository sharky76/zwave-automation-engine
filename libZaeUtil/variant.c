#include "variant.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stack.h"
#include "hash.h"

static int  auto_inc_data_type = DT_USER+1;

typedef struct variant_converter_t
{
    VariantDataType type;
    const void* (*converter_cb)(const struct variant_t*);
} variant_converter_t;

static hash_table_t*  variant_string_converter_table;

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

void variant_delete_list(void* ptr)
{
    stack_free((variant_stack_t*)ptr);
}

int variant_compare_bool(const variant_t* v, const variant_t* other)
{
    return !(v->storage.bool_data == other->storage.bool_data);
}

int variant_compare_float(const variant_t* v, const variant_t* other)
{
    return (v->storage.double_data < other->storage.double_data) ? -1 : 
        (v->storage.double_data > other->storage.double_data);
}

int variant_compare_int(const variant_t* v, const variant_t* other)
{
    return (v->storage.int_data < other->storage.int_data)? -1 : 
        (v->storage.int_data > other->storage.int_data);
}

int variant_compare_byte(const variant_t* v, const variant_t* other)
{
    return (v->storage.byte_data < other->storage.byte_data)? -1 :
        (v->storage.byte_data > other->storage.byte_data);
}

int variant_compare_string(const variant_t* v, const variant_t* other)
{
    //printf("variant_compare_string: compare %s with %s\n", variant_get_string(v), variant_get_string(other));
    return strcmp(variant_get_string(v), variant_get_string(other));
}

int variant_compare_ptr(const variant_t* v, const variant_t* other)
{
    return (v->storage.ptr_data < other->storage.ptr_data)? -1 :
        (v->storage.ptr_data > other->storage.ptr_data);
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
        new_variant->compare_cb = &variant_compare_int;
        new_variant->storage.int_data = (int32_t)data;
        break;
    case DT_STRING:
        new_variant->compare_cb = &variant_compare_string;
        new_variant->storage.string_data = (char*)data;
        new_variant->delete_cb = &variant_delete_default;
        break;
    case DT_PTR:
        new_variant->compare_cb = &variant_compare_ptr;
        new_variant->storage.ptr_data = data;
        new_variant->delete_cb = &variant_delete_default;
        break;
    case DT_LIST:
        new_variant->delete_cb = &variant_delete_list;
        new_variant->storage.ptr_data = data;
        new_variant->compare_cb = &variant_compare_ptr;
        break;
    default:
        new_variant->storage.ptr_data = data;
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

variant_t*  variant_create_byte(int type, int8_t data)
{
    variant_t* new_variant = (variant_t*)calloc(1, sizeof(variant_t));
    new_variant->type = type;
    new_variant->delete_cb = NULL;
    new_variant->ref_count = 1;

    new_variant->storage.byte_data = data;
    new_variant->compare_cb = &variant_compare_byte;

    return new_variant;
}

variant_t*  variant_create_bool(bool data)
{
    variant_t* new_variant = variant_create(DT_BOOL, (void*)data); //(variant_t*)malloc(sizeof(variant_t));
    return new_variant;
}

variant_t*  variant_create_ptr(int type, void* ptr, void (*delete_cb)(void* arg))
{
    if(NULL == ptr)
    {
        return NULL;
    }

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
    variant_t* new_variant = (variant_t*)calloc(1, sizeof(variant_t));
    new_variant->type = DT_FLOAT;
    new_variant->delete_cb = NULL;
    new_variant->ref_count = 1;

    new_variant->storage.double_data = data;
    new_variant->compare_cb = &variant_compare_float;

    return new_variant;
}

variant_t*  variant_create_string(const char* string)
{
    return variant_create(DT_STRING, (void*)string);
}

variant_t*  variant_create_list(variant_stack_t* list)
{
    return variant_create(DT_LIST, list);
}

void        variant_free(variant_t* variant)
{
    if(NULL == variant)
    {
        return;
    }

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

int8_t      variant_get_byte(variant_t* variant)
{
    return variant->storage.byte_data;
}

float       variant_get_float(variant_t* variant)
{
    return variant->storage.double_data;
}

const char* variant_get_string(const variant_t* variant)
{
    return (NULL != variant && variant->type == DT_STRING)? (const char*)variant->storage.string_data : NULL;
}

void*       variant_get_ptr(variant_t* variant)
{
    return (NULL != variant)? variant->storage.ptr_data : NULL;
}

variant_t*  variant_get_variant(variant_t* variant)
{
    variant_t* ret = (variant_t*)variant->storage.ptr_data;
    ret->ref_count++;

    return ret;
}

variant_t*  variant_clone(variant_t* variant)
{
    variant->ref_count++;
    return variant;
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

/**
 * 
 * 
 * @author alex (2/18/2016)
 *  
 * Return -1 if v1 is less than v2, 0 if they are equal, 1 if v1 
 * more than v2 
 *  
 *  
 * @param v1 
 * @param v2 
 * 
 * @return int 
 */
int         variant_compare(variant_t* v1, variant_t* v2)
{
    if(NULL != v1->compare_cb)
    {
        return v1->compare_cb(v1, v2);
    }

    return 0;
}

bool        variant_is_null(variant_t* variant)
{
    return variant->storage.ptr_data == 0;
}

#include "event_log.h"

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
            int n = snprintf(*string, 12, "%d", variant_get_int(variant));
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
    case DT_FLOAT:
        {
            *string = (char*)calloc(6, sizeof(char));
            snprintf(*string, 5, "%f", variant_get_float(variant));
        }
        break;
    case DT_LIST:
        {
            variant_stack_t* list = (variant_stack_t*)variant_get_ptr(variant);
            *string = (char*)calloc(list->count * 256, sizeof(char));

            char* br = "\r\n";

            stack_for_each(list, list_entry_variant)
            {
                char* string_entry;
                if(variant_to_string(list_entry_variant, &string_entry))
                {
                    strncat(*string, string_entry, 253);
                    strncat(*string, br, 2);
                    free(string_entry);
                }
            }
        }
        break;
    default:
        {
            variant_t* convert_cb_var = variant_hash_get(variant_string_converter_table, variant->type);
    
            if(NULL != convert_cb_var)
            {
                void (*ToStringConverter)(const struct variant_t*, char**);
                ToStringConverter = VARIANT_GET_PTR(void, convert_cb_var);
                ToStringConverter(variant, string);
                retVal = true;
            }
            else
            {
                retVal = false;
            }
        }
    }

    return retVal;
}

variant_t*  variant_add(variant_t* arg1, variant_t* arg2)
{
    if(arg1->type == DT_STRING || arg2->type == DT_STRING)
    {
        char* str1 = NULL;
        char* str2 = NULL;
        if(variant_to_string(arg1, &str1) && variant_to_string(arg2, &str2))
        {
            char* str_res = calloc(strlen(str1) + strlen(str2) + 1, sizeof(char));
            if(NULL != str_res)
            {
                strcat(str_res, str1);
                strcat(str_res, str2);
                free(str1);
                free(str2);
                return variant_create_string(str_res);
            }
        }

        free(str1);
        free(str2);
        return NULL;
    }

    switch(arg1->type)
    {
    case DT_INT8:
        return variant_create_byte(DT_INT8, variant_get_byte(arg1) + variant_get_byte(arg2));
        break;
    case DT_INT32:
    case DT_INT64:
        return variant_create_int32(DT_INT32, variant_get_int(arg1) + variant_get_int(arg2));
        break;
    case DT_FLOAT:
        return variant_create_float(variant_get_float(arg1) + variant_get_float(arg2));
        break;
    default:
        return NULL;
    }
}

variant_t*  variant_subtract(variant_t* arg1, variant_t* arg2)
{
switch(arg1->type)
    {
    case DT_INT8:
        return variant_create_byte(DT_INT8, variant_get_byte(arg1) - variant_get_byte(arg2));
        break;
    case DT_INT32:
    case DT_INT64:
        return variant_create_int32(DT_INT32, variant_get_int(arg1) - variant_get_int(arg2));
        break;
    case DT_FLOAT:
        return variant_create_float(variant_get_float(arg1) - variant_get_float(arg2));
        break;
    default:
        return NULL;
    }
}

int variant_get_next_user_type()
{
    return auto_inc_data_type++;
}

void    variant_register_converter_string(VariantDataType type, void (*converter)(struct variant_t*, char** str))
{
    if(NULL == variant_string_converter_table)
    {
        variant_string_converter_table = variant_hash_init();
    }

    variant_hash_insert(variant_string_converter_table, type, variant_create_ptr(DT_PTR, converter, NULL));
}

