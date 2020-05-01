#ifndef VARIANT_H
#define VARIANT_H

#include <stdbool.h>
#include <stdint.h>
#include <zway_json.h>

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
    DT_SENSOR,
    //DT_SENSOR_EVENT_DATA,
    //DT_SERVICE_EVENT_DATA,
    //DT_VDEV,
    //DT_VDEV_EVENT_DATA,
    DT_LIST,
    DT_EVENT_LOG_ENTRY,
    DT_NULL,

    DT_USER,
} VariantDataType;

typedef struct variant_t
{
    VariantDataType type;
    int             ref_count;

    union {
        void*           ptr_data;
        char*           string_data;
        bool            bool_data;
        int8_t          byte_data;
        int32_t         int_data;
        double          double_data;
    } storage;

    void    (*delete_cb)(void*);
    int     (*compare_cb)(const struct variant_t* v, const struct variant_t* other);
} variant_t;

typedef struct variant_stack_t variant_stack_t;

#define VARIANT_GET_PTR(_type_, _var_)  \
    (_type_*)variant_get_ptr(_var_)

variant_t*  variant_create(VariantDataType type, void* data);
void        variant_delete_default(void* ptr);
void        variant_delete_variant(void* ptr);
void        variant_delete_none(void* ptr);
variant_t*  variant_create_int32(int type, int32_t data);
variant_t*  variant_create_byte(int type, int8_t data);
variant_t*  variant_create_bool(bool data);
variant_t*  variant_create_ptr(int type, void* ptr, void (*delete_cb)(void* arg));
variant_t*  variant_create_variant(int type, variant_t* variant);
variant_t*  variant_create_float(double data);
variant_t*  variant_create_string(const char* string);
variant_t*  variant_create_list(variant_stack_t* list);
void        variant_free(variant_t* variant);

bool        variant_get_bool(variant_t* variant);
int         variant_get_int(variant_t* variant);
int8_t      variant_get_byte(variant_t* variant);
float       variant_get_float(variant_t* variant);
const char* variant_get_string(const variant_t* variant);
void*       variant_get_ptr(variant_t* variant);
variant_t*  variant_get_variant(variant_t* variant);
variant_stack_t* variant_get_list(variant_t* variant);
variant_t*  variant_clone(variant_t* variant);
bool        variant_is_null(variant_t* variant);

bool variant_to_string(variant_t* variant, char** string);

int         variant_compare(variant_t* v1, variant_t* v2);

int         variant_get_next_user_type();

variant_t*  variant_add(variant_t* arg1, variant_t* arg2);
variant_t*  variant_subtract(variant_t* arg1, variant_t* arg2);

void        variant_register_converter_string(VariantDataType source_type,  void (*converter)(struct variant_t*, char** str));

struct json_object* variant_to_json_object(variant_t* variant);
#endif // VARIANT_H
