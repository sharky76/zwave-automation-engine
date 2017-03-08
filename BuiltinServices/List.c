#include "List.h"
#include "../scene_action.h"
#include "../command_parser.h"
#include "../builtin_service_manager.h"
#include "../service_manager.h"
#include "service_args.h"
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
// List.ForEach(SurveillanceStation.GetEventList(1), SurveillanceStation.GetSnapshot)
/*
Each list item must convert to string or int and it will replace the $arg token in the expression. 
Then the expression will be compiles and run for each list item 
*/
variant_t*  foreach_impl(struct service_method_t* method, va_list args)
{
    //variant_t* list_create_variant = va_arg(args, variant_t*);
    //service_method_t* eval_method = builtin_service_manager_get_method("Expression", "Eval");
    //variant_t* list_variant = service_manager_eval_method(eval_method, list_create_variant);

    variant_t* list_variant = va_arg(args, variant_t*);
    if(list_variant->type != DT_LIST)
    {
        return variant_create_bool(false);
    }

    variant_t* expression_variant = va_arg(args, variant_t*);
    //variant_t* retval = variant_create_string("");

    variant_stack_t* list = (variant_stack_t*)variant_get_ptr(list_variant);
    const char* expression_string = variant_get_string(expression_variant);
    uint32_t key = crc32(0, "arg", 3);
    service_args_stack_create("Expression.ProcessTemplate");

    stack_for_each(list, list_item_variant)
    {
        service_args_stack_add("Expression.ProcessTemplate", key, list_item_variant);
        service_method_t* process_template_method = builtin_service_manager_get_method("Expression", "ProcessTemplate");
        //printf("Exptession to process: %s \n", variant_get_string(expression_variant));

        /*char* list_item;
        variant_to_string(list_item_variant, &list_item);
        printf("With arg: %s\n", list_item);*/

        variant_t* processed_expression = service_manager_eval_method(process_template_method, expression_variant);

        //printf("Final exptession to compile: %s\n", variant_get_string(processed_expression));
        // Now we have processed template with proper argument substitution.
        // We can remove this value from the stack:
        service_args_stack_remove("Expression.ProcessTemplate", key);
        // And eval the expression:
        if(NULL != processed_expression)
        {
            service_method_t* eval_method = builtin_service_manager_get_method("Expression", "Eval");
            variant_t* result = service_manager_eval_method(eval_method, processed_expression);
            if(NULL == result)
            {
                return variant_create_bool(false);
            }
        }
    }

    return variant_create_bool(true);
}

