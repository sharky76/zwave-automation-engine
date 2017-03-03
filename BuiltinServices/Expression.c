#include "Expression.h"
#include "../scene_action.h"
#include "../command_parser.h"
#include "service_args.h"
#include <crc32.h>
#include <ctype.h>

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
    if(variant_to_string(expression_var, &expression))
    {
        bool isOk;
        variant_stack_t* compiled = command_parser_compile_expression(expression, &isOk);
        free(expression);
        if(isOk)
        {
            return command_parser_execute_expression(compiled);
        }
    }

    return NULL;
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
                uint32_t key = crc32(0, token_buf, strlen(token_buf));
                variant_t* value_var = variant_hash_get(token_table, key);
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
                        expected_size *= 2;
                        result_string = realloc(result_string, expected_size);
                    }

                    strncat(result_string, replacement, expected_size-result_size);
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

        //variant_free(token_table_var);
    }

    // Remove last space...
    //result_string[strlen(result_string)] = 0;
    return variant_create_string(result_string);
}

variant_t*  unescape_impl(struct service_method_t* method, va_list args)
{
    variant_t*  expression_var = va_arg(args, variant_t*);
    const char* escaped_string = variant_get_string(expression_var);
    char* result_string = calloc(strlen(escaped_string), sizeof(char));

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

