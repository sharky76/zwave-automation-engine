#include "zway_json.h"

void data_leaf_to_json(json_object* root, ZDataHolder data);

void zway_json_data_holder_to_json(json_object* root, ZDataHolder data)
{
    ZDataIterator child = zdata_first_child(data);

    if(child == NULL)
    {
        data_leaf_to_json(root, data);
        return;
    }

    while (child != NULL)
    {
        data_leaf_to_json(root, child->data);
    
        ZDataIterator nested_child = zdata_first_child(child->data);
        if (nested_child != NULL)
        {
            // Nested child: create nested JSON structure
            const char* name = zdata_get_name(child->data);
            json_object* nested_data = json_object_new_object();
            zway_json_data_holder_to_json(nested_data, child->data);
            json_object_object_add(root, name, nested_data);
        }

        child = zdata_next_child(child);
    }
}


void data_leaf_to_json(json_object* root, ZDataHolder data)
{
    const char* name = zdata_get_name(data);

    ZWDataType type;
    zdata_get_type(data, &type);

    ZWBOOL bool_val;
    int int_val;
    float float_val;
    ZWCSTR str_val;
    const ZWBYTE *binary;
    const int *int_arr;
    const float *float_arr;
    const ZWCSTR *str_arr;
    size_t len, i;

    switch (type) 
    {
    case Empty:
        json_object_object_add(root, name, json_object_new_string("Empty"));
            break;
        case Boolean:
            zdata_get_boolean(data, &bool_val);
            json_object_object_add(root, name, json_object_new_boolean(bool_val));
            break;
        case Integer:
            zdata_get_integer(data, &int_val);
            json_object_object_add(root, name, json_object_new_int(int_val));
            break;
        case Float:
            zdata_get_float(data, &float_val);
            json_object_object_add(root, name, json_object_new_double(float_val));
            break;
    case String:
            zdata_get_string(data, &str_val);
            json_object_object_add(root, name, json_object_new_string(str_val));
            break;
        case Binary:
            zdata_get_binary(data, &binary, &len);
            json_object* bin_array = json_object_new_array();
            for(int i = 0; i < len; i++)
            {
                json_object_array_add(bin_array, json_object_new_int(binary[i]));
            }

            json_object_object_add(root, name, bin_array);
            break;
        case ArrayOfInteger:
            zdata_get_integer_array(data, &int_arr, &len);
            json_object* int_array = json_object_new_array();
            for(int i = 0; i < len; i++)
            {
                json_object_array_add(int_array, json_object_new_int(int_arr[i]));
            }
            json_object_object_add(root, name, int_array);
            break;
        case ArrayOfFloat:
            zdata_get_float_array(data, &float_arr, &len);
            json_object* float_array = json_object_new_array();
            for(int i = 0; i < len; i++)
            {
                json_object_array_add(float_array, json_object_new_double(float_arr[i]));
            }
            json_object_object_add(root, name, float_array);
            break;
        case ArrayOfString:
            zdata_get_string_array(data, &str_arr, &len);
            json_object* string_array = json_object_new_array();
            for(int i = 0; i < len; i++)
            {
                json_object_array_add(string_array, json_object_new_string(str_arr[i]));
            }
            json_object_object_add(root, name, string_array);
            break;
        default:
            break;
    }
}
