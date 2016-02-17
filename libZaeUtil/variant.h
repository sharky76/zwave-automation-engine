#ifndef VARIANT_H
#define VARIANT_H

#include <stdbool.h>
#include <stdint.h>

/*
This is a weak attempt to define a variant type in C
*/

typedef enum VariantDataType
{
    DT_BOOL,
    DT_INT8,
    DT_INT32,
    DT_INT64,
    DT_FLOAT,
    DT_STRING,
    DT_PTR,
    DT_EVENT,
    DT_SERVICE_EVENT_DATA,
    DT_NULL,

    DT_USER,
} VariantDataType;

typedef struct variant_t
{
    VariantDataType type;
    int             ref_count;

    union {
        void*           ptr_data;
        bool            bool_data;
        int32_t         int_data;
        double          double_data;
    } storage;

    void    (*delete_cb)(void*);
    bool    (*compare_cb)(const struct variant_t* v, const struct variant_t* other);
} variant_t;

variant_t*  variant_create(VariantDataType type, void* data);
void        variant_delete_default(void* ptr);
void        variant_delete_variant(void* ptr);
void        variant_delete_none(void* ptr);
variant_t*  variant_create_int32(int type, int32_t data);
variant_t*  variant_create_bool(bool data);
variant_t*  variant_create_ptr(int type, void* ptr, void (*delete_cb)(void* arg));
variant_t*  variant_create_variant(int type, variant_t* variant);
variant_t*  variant_create_float(double data);
variant_t*  variant_create_string(const char* string);
void        variant_free(variant_t* variant);

bool        variant_get_bool(variant_t* variant);
int         variant_get_int(variant_t* variant);
float       variant_get_float(variant_t* variant);
const char* variant_get_string(variant_t* variant);
void*       variant_get_ptr(variant_t* variant);
variant_t*  variant_get_variant(variant_t* variant);
bool        variant_is_null(variant_t* variant);

bool variant_to_string(variant_t* variant, char** string);

int         variant_compare(variant_t* v1, variant_t* v2);

int         variant_get_next_user_type();

#endif // VARIANT_H
