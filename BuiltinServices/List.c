#include "List.h"
#include "../scene_action.h"
#include "../command_parser.h"
#include "../builtin_service_manager.h"
#include "../service_manager.h"
#include <crc32.h>
#include <stack.h>
#include <ctype.h>

variant_t*  foreach_impl(struct service_method_t* method, va_list args);

builtin_service_t*  list_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "List", "List manipulation methods");
    builtin_service_add_method(service, "ForEach", "Apply method for each list item", 2, foreach_impl); // 1 arg - list, 2 arg - expression with list item as argument. Argument place is
                                                                                                        // specified by "$arg" token
    return service;
}


// List.ForEach(SurveillanceStation.GetCameraList(), 'Conditional.If(SurveillanceStation.GetEvents($arg) > 0, SMS.Send(Camera  + $arg +  has detected motion.), False)'
variant_t*  foreach_impl(struct service_method_t* method, va_list args)
{
    variant_t* list_variant = va_arg(args, variant_t*);
    variant_t* expression_variant = va_arg(args, variant_t*);

    variant_stack_t* list = (variant_stack_t*)variant_get_ptr(list_variant);
    const char* expression_string = variant_get_string(expression_variant);

    builtin_service_stack_create("Expression.ProcessTemplate");
    uint32_t key = crc32(0, "arg", 3);

    stack_for_each(list, list_item_variant)
    {
        // Replace each '$' with list item
        char* list_item_string;
        builtin_service_stack_add("Expression.ProcessTemplate", key, list_item_variant);

        service_method_t* process_tmpl_method = builtin_service_manager_get_method("Expression", "ProcessTemplate");
        variant_t* processed_expression = service_manager_eval_method(process_tmpl_method, expression_variant);
        builtin_service_stack_remove("Expression.ProcessTemplate", key);


        service_method_t* unescape_method = builtin_service_manager_get_method("Expression", "Unescape");
        variant_t* unescaped_expr = service_manager_eval_method(unescape_method, processed_expression);

        printf("foreach_impl: %s\n", variant_get_string(unescaped_expr));

        bool isOk;
        variant_stack_t* compiled = command_parser_compile_expression(variant_get_string(processed_expression), &isOk);
        if(isOk)
        {
            variant_t* result = command_parser_execute_expression(compiled);
            variant_free(result);
        }
        stack_free(compiled);
    }

    return variant_create_bool(true);
}

