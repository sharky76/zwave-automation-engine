#include "Conditional.h"
#include "../scene_action.h"
#include "../command_parser.h"
#include <crc32.h>
#include <ctype.h>

variant_t*  if_impl(struct service_method_t* method, va_list args);

builtin_service_t*  conditional_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "Conditional", "Conditional methods");
    builtin_service_add_method(service, "If", "IF condition", 3, if_impl);
    return service;
}

variant_t*  if_impl(struct service_method_t* method, va_list args)
{
    variant_t* condition_expr = va_arg(args, variant_t*);
    variant_t* true_case = va_arg(args, variant_t*);
    variant_t* false_case = va_arg(args, variant_t*);

    //char* expression;
    //if(variant_to_string(condition_expr, &expression))
    {
        bool isOk;
        //variant_stack_t* compiled = command_parser_compile_expression(expression, &isOk);
    
        //if(isOk)
        {
            //variant_t* res = command_parser_execute_expression(compiled);
            
            if(condition_expr->type == DT_BOOL)
            {
                char* case_expression;
                if(variant_get_bool(condition_expr))
                {
                    if(variant_to_string(true_case, &case_expression))
                    {
                        free(case_expression);
                        return variant_create_string(case_expression);
                    }
                }
                else
                {
                    if(variant_to_string(false_case, &case_expression))
                    {
                        free(case_expression);
                        return variant_create_string(case_expression);
                    }
                }
            }
        }
    }

    return NULL;
}

