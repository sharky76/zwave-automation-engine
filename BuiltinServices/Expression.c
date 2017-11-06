#include "Expression.h"
#include "../scene_action.h"
#include "../command_parser.h"
#include "service_args.h"
#include <crc32.h>
#include <ctype.h>
#include "logger.h"

USING_LOGGER(BuiltinService)

variant_t*  eval_impl(struct service_method_t* method, va_list args);
variant_t*  process_template_impl(struct service_method_t* method, va_list args);
variant_t*  unescape_impl(struct service_method_t* method, va_list args);

builtin_service_t*  expression_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "Expression", "Expression management methods");
    builtin_service_add_method(service, "Eval", "Evaluate string expression", 1, eval_impl);
    builtin_service_add_method(service, "ProcessTemplate", "Process template string", 1, process_template_impl);
    builtin_service_add_method(service, "Unescape", "Unescape expression string", 1, unescape_impl);

    return service;
}

variant_t*  eval_impl(struct service_method_t* method, va_list args)
{
    variant_t* expression_var = va_arg(args, variant_t*);
    char* expression;
    variant_t* result = NULL;

    if(variant_to_string(expression_var, &expression))
    {
        bool isOk;
        variant_stack_t* compiled = command_parser_compile_expression(expression, &isOk);
        free(expression);
        if(isOk)
        {
            result = command_parser_execute_expression(compiled);
        }
        stack_free(compiled);
    }

    return result;
}

variant_t*  process_template_impl(struct service_method_t* method, va_list args)
{
    // First, get stack for this method
    service_args_stack_t* service_stack = service_args_stack_get("Expression.ProcessTemplate");
    variant_t*  template_var = va_arg(args, variant_t*);
    char* result_string = NULL;

    if(NULL != service_stack)
    {
        //variant_t* token_table_var = stack_pop_front(service_stack->stack);
        hash_table_t*   token_table = (hash_table_t*)service_stack->data_storage;//variant_get_ptr(token_table_var);

        // Replace all tokens starting with "$" with matching values from token table
        char* template = (char*)variant_get_string(template_var);
        if(NULL == template)
        {
            return NULL;
        }

        char* tokens = template;
        int initial_len = strlen(template);
    
        int result_size = 0;

        // Initially create enough space for 2x template length
        int expected_size = initial_len*2;
        result_string = calloc(expected_size, sizeof(char));
    
        //char* tok = strtok(tokens, " ");
    
        while(*tokens)
        {
            if(*tokens == '$')
            {
                // Read tokens until not isalpha
                ++tokens;
                char   token_buf[128] = {0};
                int    buf_idx = 0;
                while(isalnum(*tokens) && buf_idx < 128)
                {
                    token_buf[buf_idx] = *tokens;
                    ++buf_idx;
                    ++tokens;
                }
                --tokens;
                variant_t* value_var = service_args_get_value(service_stack, token_buf);
                if(NULL != value_var)
                {
                    //stack_push_back(result_stack, variant_create_string(strdup(variant_to_string(value_var))));
                    char* replacement;
                    /*if(value_var->type == DT_STRING)
                    {
                        replacement = calloc(2 + strlen(variant_get_string(value_var)), sizeof(char));
                        sprintf(replacement, "'%s'", variant_get_string(value_var));
                    }
                    else*/
                    {
                        variant_to_string(value_var, &replacement);
                    }

                    int replacement_len = strlen(replacement);
                    if(result_size + replacement_len + 1 >= expected_size)
                    {
                        // Need to realloc our string...
                        expected_size = (result_size + replacement_len + 1) * 2;
                        result_string = realloc(result_string, expected_size);
                        if(NULL == result_string)
                        {
                            LOG_ERROR(BuiltinService, "Memory allocation error, tried %d bytes", expected_size);
                            return NULL;
                        }
                        result_string[result_size] = 0;
                        //printf("new expect size = %d\n, result: %d", expected_size, result_size + replacement_len + 1);

                    }

                    
                    memcpy((char*)result_string+result_size, replacement, replacement_len);
                    //strcat(result_string, " ");
                    result_size += replacement_len;
                    free(replacement);
                }
                else
                {
                    //strncat(result_string, tok, expected_size-result_size);
                    //strcat(result_string, " ");
                    result_string[result_size] = *tokens;
                    ++result_size;
                }
            }
            else
            {
                result_string[result_size] = *tokens;
                ++result_size;
            }
    
            ++tokens;
        }
        result_string[result_size] = 0;
        LOG_DEBUG(BuiltinService, "Converted template: %s", result_string);
        //variant_free(token_table_var);
        return variant_create_string(result_string);
    }

    // Remove last space...
    //result_string[strlen(result_string)] = 0;
    return NULL;
}

variant_t*  unescape_impl(struct service_method_t* method, va_list args)
{
    variant_t*  expression_var = va_arg(args, variant_t*);

    const char* escaped_string = variant_get_string(expression_var);
    if(NULL != escaped_string)
    {
        char* result_string = calloc(strlen(escaped_string)+1, sizeof(char));
    
        int j = 0;
        for(int i = 0; i < strlen(escaped_string); i++)
        {
            if(escaped_string[i] != '\\')
            {
                result_string[j++] = escaped_string[i];
            }
        }
    
        return variant_create_string(result_string);
    }

    return expression_var;
}

