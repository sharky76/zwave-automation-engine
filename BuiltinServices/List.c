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
variant_t*  has_item_impl(struct service_method_t* method, va_list args);

builtin_service_t*  list_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "List", "List manipulation methods");
    builtin_service_add_method(service, "ForEach", "Apply method for each list item", 2, foreach_impl); // 1 arg - list, 2 arg - expression with list item as argument. Argument place is
                                                                                                        // specified by "$arg" token
    builtin_service_add_method(service, "HasItem", "Return true if an item is part of the list", 2, has_item_impl);
    return service;
}


// List.ForEach(SurveillanceStation.GetCameraList(), 'Conditional.If(SurveillanceStation.GetEvents($arg) > 0, SMS.Send(Camera  + $arg +  has detected motion.), False)'
// List.ForEach(SurveillanceStation.GetEventList(1), SurveillanceStation.GetSnapshot)
/*
Each list item must convert to string or int and it will replace the $arg token in the expression. 
Then the expression will be compiles and run for each list item 
*/

/*****************************************************************//*

    This method parses and evaluates the value before inserting it
    into Expression.ProcessTemplate argument stack.
*******************************************************************/
void evaluate_argument_value(uint32_t key, variant_t* value, void* arg)
{
    // Now we want to unescape the expression to restore proper quotes...
    service_method_t* unescape_method = builtin_service_manager_get_method("Expression", "Unescape");
    variant_t* unescaped_expression = service_manager_eval_method(unescape_method, value);

    // Now we want to substitute $arg with the actial list item
    service_method_t* process_template_method = builtin_service_manager_get_method("Expression", "ProcessTemplate");
    variant_t* processed_expression = service_manager_eval_method(process_template_method, unescaped_expression);
    variant_free(unescaped_expression);

    // Now lets execute the expression
    service_method_t* eval_method = builtin_service_manager_get_method("Expression", "Eval");
    variant_t* result = service_manager_eval_method(eval_method, processed_expression);
    variant_free(processed_expression);
    if(NULL != result)
    {
        // Ok, we have results, lets insert it into Expression.ProcessTemplate stack
        service_args_stack_add_with_key("Expression.ProcessTemplate", key, result);
    }
}


variant_t*  foreach_impl(struct service_method_t* method, va_list args)
{
    // List uses args stack slightly differently. It compiles and executes value
    // for each stack entry and uses those values as argument stack for the expression it runs
    service_args_stack_t* service_stack = service_args_stack_get("List.ForEach");

    variant_t* list_variant = va_arg(args, variant_t*);
    if(list_variant->type != DT_LIST)
    {
        return variant_create_bool(false);
    }

    variant_t* expression_variant = va_arg(args, variant_t*);
    
    variant_stack_t* list = (variant_stack_t*)variant_get_ptr(list_variant);

    // Now we want to unescape the expression to restore proper quotes...
    service_method_t* unescape_method = builtin_service_manager_get_method("Expression", "Unescape");
    variant_t* unescaped_expression = service_manager_eval_method(unescape_method, expression_variant);

    service_args_stack_create("Expression.ProcessTemplate");

    variant_t* retVal = variant_create_string(strdup(""));
    stack_for_each(list, list_item_variant)
    {
        service_args_stack_add("Expression.ProcessTemplate", "arg", variant_clone(list_item_variant));

        if(NULL != service_stack)
        {
            // We have list item, lets use it to compile value from argument stack...
            service_args_for_each(service_stack, evaluate_argument_value, NULL);
        }

        service_method_t* process_template_method = builtin_service_manager_get_method("Expression", "ProcessTemplate");
        variant_t* processed_expression = service_manager_eval_method(process_template_method, unescaped_expression);

        // Now we have processed template with proper argument substitution.
        // We can remove this value from the stack:
        service_args_stack_remove("Expression.ProcessTemplate", "arg");
        // And eval the expression:
        if(NULL != processed_expression)
        {
            service_method_t* eval_method = builtin_service_manager_get_method("Expression", "Eval");
            variant_t* result = service_manager_eval_method(eval_method, processed_expression);
            variant_free(processed_expression);
            if(NULL == result)
            {
                service_args_stack_destroy("Expression.ProcessTemplate");
                variant_free(unescaped_expression);
                return variant_create_bool(false);
            }
            else
            {
                variant_t* v = retVal;
                retVal = variant_add(v, result);
                variant_free(v);
                variant_free(result);
            }
        }
    }

    service_args_stack_destroy("Expression.ProcessTemplate");
    variant_free(unescaped_expression);

    //return variant_create_bool(true);
    return retVal;
}

variant_t*  has_item_impl(struct service_method_t* method, va_list args)
{
    variant_t* list_variant = va_arg(args, variant_t*);
    if(list_variant->type != DT_LIST)
    {
        return variant_create_bool(false);
    }

    variant_t* item_variant = va_arg(args, variant_t*);
    // Compile item variant (if needed)
    service_method_t* eval_method = builtin_service_manager_get_method("Expression", "Eval");
    variant_t* compiled_item = service_manager_eval_method(eval_method, item_variant);

    variant_stack_t* list = (variant_stack_t*)variant_get_ptr(list_variant);

    stack_for_each(list, list_item_variant)
    {
        if(variant_compare(compiled_item, list_item_variant) == 0)
        {
            variant_free(compiled_item);
            return variant_create_bool(true);
        }
    }

    variant_free(compiled_item);
    return variant_create_bool(false);
}

