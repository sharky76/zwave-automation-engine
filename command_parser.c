#include "command_parser.h"
#include "resolver.h"
#include "parser_dfa.h"
#include "service_manager.h"
#include "builtin_service_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <logger.h>
#include "vdev_manager.h"

/*
While there are tokens to be read:
Read a token.
If the token is a number, then add it to the output queue.
If the token is a function token, then push it onto the stack.
If the token is a function argument separator (e.g., a comma):
Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue. If no left parentheses are encountered, either the separator was misplaced or parentheses were mismatched.
If the token is an operator, o1, then:
while there is an operator token, o2, at the top of the operator stack, and either
o1 is left-associative and its precedence is less than or equal to that of o2, or
o1 is right associative, and has precedence less than that of o2,
then pop o2 off the operator stack, onto the output queue;
push o1 onto the operator stack.
If the token is a left parenthesis (i.e. "("), then push it onto the stack.
If the token is a right parenthesis (i.e. ")"):
Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
Pop the left parenthesis from the stack, but not onto the output queue.
If the token at the top of the stack is a function token, pop it onto the output queue.
If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
When there are no more tokens to read:
While there are still operator tokens in the stack:
If the operator token on the top of the stack is a parenthesis, then there are mismatched parentheses.
Pop the operator onto the output queue.
Exit.
*/

static reserved_word_t  reserved_words[] = {
    {"True", 1},
    {"False", 0},
    {"On", 1},
    {"Off", 0},
    {0, 0}
};

bool process_string_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue);
void process_string_operand(const char* ch, variant_stack_t* operand_queue);
bool process_service_method_operator(const char* class, const char* method, variant_stack_t* operator_stack);
bool process_builtin_method_operator(const char* class, const char* method, variant_stack_t* operator_stack);
bool process_device_function_operator(const char* device, const char* method, variant_stack_t* operator_stack);
bool process_vdev_method_operator(const char* device, const char* method, variant_stack_t* operator_stack);
bool process_digit_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue);

void process_operator_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token);
bool process_parenthesis_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token);
bool process_comma_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue);

bool eval(variant_stack_t* work_stack, variant_t* op);

/*
 
    State machine state implementation
 
    STATE_START,
    STATE_ALPHA,
    STATE_DIGIT,
    STATE_RIGHT_PAREN,
    STATE_LEFT_PAREN,
    STATE_COMMA,
    STATE_AND,
    STATE_OR,
    STATE_CMP,
    STATE_ERROR,
    STATE_END,
    STATE_INVALID
 
*/

State    state_start(state_context_t* state_context, void* priv);
State    state_alpha(state_context_t* state_context, void* priv);
State    state_digit(state_context_t* state_context, void* priv);
State    state_right_paren(state_context_t* state_context, void* priv);
State    state_left_paren(state_context_t* state_context, void* priv);
State    state_comma(state_context_t* state_context, void* priv);
State    state_and(state_context_t* state_context, void* priv);
State    state_or(state_context_t* state_context, void* priv);
State    state_plus(state_context_t* state_context, void* priv);
State    state_minus(state_context_t* state_context, void* priv);
State    state_unary_minus(state_context_t* state_context, void* priv);
State    state_cmp(state_context_t* state_context, void* priv);
State    state_less(state_context_t* state_context, void* priv);
State    state_more(state_context_t* state_context, void* priv);
State    state_capture_string(state_context_t* state_context, void* priv);
State    state_error(state_context_t* state_context, void* priv);
State    state_end(state_context_t* state_context, void* priv);
State    state_invalid(state_context_t* state_context, void* priv);
State    state_accept_token(state_context_t* state_context, void* priv);

typedef struct parser_data_t
{
    variant_stack_t* operator_stack;
    variant_stack_t* operand_queue;
} parser_data_t;

bool    process_state_transition(State current_state, state_context_t* state_context, void* priv);

static state_descriptor_t state_map[] = {
    {STATE_START,   &state_start},
    {STATE_ALPHA,   &state_alpha},
    {STATE_DIGIT,   &state_digit},
    {STATE_RIGHT_PAREN, &state_right_paren},
    {STATE_LEFT_PAREN,  &state_left_paren},
    {STATE_COMMA,   &state_comma},
    {STATE_AND,     &state_and},
    {STATE_OR,      &state_or},
    {STATE_PLUS,    &state_plus},
    {STATE_MINUS,   &state_minus},
    {STATE_UNARY_MINUS, &state_unary_minus},
    {STATE_CMP,     &state_cmp},
    {STATE_LESS,    &state_less},
    {STATE_MORE,    &state_more},
    {STATE_CAPTURE_STRING,  &state_capture_string},
    {STATE_ACCEPT_TOKEN, &state_accept_token},
    {STATE_ERROR,   &state_error},
    {STATE_END,     &state_end},
    {STATE_INVALID, &state_invalid}
};

DECLARE_LOGGER(Parser)

bool process_state_transition(State current_state, state_context_t* state_context, void* priv)
{
    parser_data_t* parser_data = (parser_data_t*)priv;
    bool retVal = true;

    // Here is the problem!!! Perenthesis could come in numbers!!!
    //if(state_context->current_state != current_state)
    {
        // Ok, we arrived here from another state, meaning we have some data from 
        // previous state to process

        switch(state_context->current_state)
        {
        case STATE_CAPTURE_STRING:
            process_string_operand(state_context->current_data_buf, parser_data->operand_queue);
            *state_context->current_data_buf = 0;
            break;
        case STATE_ALPHA:
            retVal = process_string_token(state_context->current_data_buf, parser_data->operator_stack, parser_data->operand_queue);
            *state_context->current_data_buf = 0;
            break;
        case STATE_DIGIT:
            if(process_digit_token(state_context->current_data_buf, parser_data->operator_stack, parser_data->operand_queue))
            {
                *state_context->current_data_buf = 0;
            }
            break;
        case STATE_RIGHT_PAREN:
            retVal = process_parenthesis_token(parser_data->operator_stack, parser_data->operand_queue, OP_RIGHT_PARETHESIS);
            break;
        case STATE_LEFT_PAREN:
            retVal = process_parenthesis_token(parser_data->operator_stack, parser_data->operand_queue, OP_LEFT_PARETHESIS);
            break;
        case STATE_COMMA:
            retVal = process_comma_token(parser_data->operator_stack, parser_data->operand_queue);
            break;
        case STATE_AND:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_AND);
            break;
        case STATE_OR:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_OR);
            break;
        case STATE_PLUS:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_PLUS);
            break;
        case STATE_MINUS:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_MINUS);
            break;
        case STATE_UNARY_MINUS:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_UNARY_MINUS);
            break;
        case STATE_CMP:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_CMP);
            break;
         case STATE_LESS:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_LESS);
            break;
         case STATE_MORE:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_MORE);
            break;
        case STATE_END:
        case STATE_START:
        case STATE_ACCEPT_TOKEN:
        case STATE_ERROR:
            break;
        default:
            {
                LOG_ERROR(Parser, "Error, invalid transition");
            }
        }

        state_context->current_state = current_state;
    }
    
    return retVal;
}

State    state_start(state_context_t* state_context, void* priv)
{
    bool ret_val = true;

    if(state_context->current_state != STATE_START)
    {
        ret_val = process_state_transition(STATE_START, state_context, priv);
    }

    return (ret_val)? parser_dfa_next_state(STATE_START, parser_dfa_read_next_token(state_context)) : STATE_ERROR;
}

State    state_accept_token(state_context_t* state_context, void* priv)
{
    bool ret_val = true;

    if(state_context->current_state != STATE_ACCEPT_TOKEN)
    {
        ret_val = process_state_transition(STATE_ACCEPT_TOKEN, state_context, priv);
    }

    return (ret_val)? parser_dfa_next_state(STATE_ACCEPT_TOKEN, parser_dfa_read_next_token(state_context)) : STATE_ERROR;
}


State    state_alpha(state_context_t* state_context, void* priv)
{
    bool ret_val = true;

    if(state_context->current_state != STATE_ALPHA)
    {
        ret_val = process_state_transition(STATE_ALPHA, state_context, priv);
    }

    if(state_context->current_token != A_SPACE)
    {
        strncat(state_context->current_data_buf, state_context->parsed_token, state_context->parse_cycle);
    }
    //*state_context->parsed_token = '\0';

    return (ret_val)? parser_dfa_next_state(STATE_ALPHA, parser_dfa_read_next_token(state_context)) : STATE_ERROR;
}

State    state_digit(state_context_t* state_context, void* priv)
{
    bool ret_val = true;

    if(state_context->current_state != STATE_DIGIT)
    {
        process_state_transition(STATE_DIGIT, state_context, priv);
    }

    if(state_context->current_token != A_SPACE)
    {
        strncat(state_context->current_data_buf, state_context->parsed_token, state_context->parse_cycle);
    }

    //*state_context->parsed_token = '\0';

    return (ret_val)? parser_dfa_next_state(STATE_DIGIT, parser_dfa_read_next_token(state_context)) : STATE_ERROR;
}

State    state_right_paren(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_RIGHT_PAREN, state_context, priv))? 
            parser_dfa_next_state(STATE_RIGHT_PAREN, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_left_paren(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_LEFT_PAREN, state_context, priv))?
            parser_dfa_next_state(STATE_LEFT_PAREN, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_comma(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_COMMA, state_context, priv))?
            parser_dfa_next_state(STATE_COMMA, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_and(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_AND, state_context, priv))?
            parser_dfa_next_state(STATE_AND, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_or(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_OR, state_context, priv))?
            parser_dfa_next_state(STATE_OR, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_plus(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_PLUS, state_context, priv))?
            parser_dfa_next_state(STATE_PLUS, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_minus(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_MINUS, state_context, priv))?
                parser_dfa_next_state(STATE_MINUS, parser_dfa_read_next_token(state_context)) :
                STATE_ERROR;
}

State    state_unary_minus(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_UNARY_MINUS, state_context, priv))?
                parser_dfa_next_state(STATE_UNARY_MINUS, parser_dfa_read_next_token(state_context)) :
                STATE_ERROR;
}

State    state_cmp(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_CMP, state_context, priv))?
            parser_dfa_next_state(STATE_CMP, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_less(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_LESS, state_context, priv))?
            parser_dfa_next_state(STATE_LESS, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_more(state_context_t* state_context, void* priv)
{
    return (process_state_transition(STATE_MORE, state_context, priv))?
            parser_dfa_next_state(STATE_MORE, parser_dfa_read_next_token(state_context)) :
            STATE_ERROR;
}

State    state_capture_string(state_context_t* state_context, void* priv)
{
    bool ret_val = true;

    if(state_context->current_state != STATE_CAPTURE_STRING)
    {
        ret_val = process_state_transition(STATE_CAPTURE_STRING, state_context, priv);
    }
    else
    {
        strncat(state_context->current_data_buf, state_context->parsed_token, state_context->parse_cycle);
        
    }

    //*state_context->parsed_token = '\0';

    return (ret_val)? parser_dfa_next_state(STATE_CAPTURE_STRING, parser_dfa_read_next_token(state_context)) : STATE_ERROR;
}

State    state_error(state_context_t* state_context, void* priv)
{
    LOG_ERROR(Parser, "Error parsing expression: %s", state_context->expression);
    state_context->current_state = STATE_ERROR;
    return STATE_ERROR;
}

State    state_end(state_context_t* state_context, void* priv)
{
    bool retVal = process_state_transition(STATE_END, state_context, priv);

    parser_data_t* parser_data = (parser_data_t*)priv;
    while(parser_data->operator_stack->count > 0)
    {
        variant_t* data = stack_pop_front(parser_data->operator_stack);
        if(data->type == T_PARETHESIS)
        {
            LOG_ERROR(Parser, "Mismatched parenthesis");
            variant_free(data);
        }
        else
        {
            stack_push_back(parser_data->operand_queue, data);
        }
    }

    return (retVal)? STATE_END : STATE_ERROR;
}

State    state_invalid(state_context_t* state_context, void* priv)
{
    LOG_ERROR(Parser, "Invalid token received: %s", state_context->parsed_token);
    state_context->current_state = STATE_INVALID;
    return STATE_ERROR;
}

variant_stack_t*    command_parser_compile_expression(const char* expression, bool* isOk)
{
    variant_stack_t* operand_queue = stack_create();
    variant_stack_t* operator_stack = stack_create();

    LOG_DEBUG(Parser, "Parsing expression: %s", expression);

    parser_data_t   parser_data;
    parser_data.operator_stack = operator_stack;
    parser_data.operand_queue = operand_queue;

    *isOk = parser_dfa_run(STATE_START, STATE_END, STATE_ERROR, state_map, sizeof(state_map) / sizeof(state_descriptor_t), expression, &parser_data);

    stack_free(operator_stack);
    return operand_queue;
}

/*
If the token is an operator, o1, then:
while there is an operator token, o2, at the top of the operator stack, and either
o1 is left-associative and its precedence is less than or equal to that of o2, or
o1 is right associative, and has precedence less than that of o2,
then pop o2 off the operator stack, onto the output queue;
push o1 onto the operator stack.
*/

// NOTE: All our operators are left-associative except UNARY-MINUS
void process_operator_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token)
{
    variant_t* stack_front = stack_peek_front(operator_stack);
    while(NULL != stack_front)
    {
        operator_t* stacked_operator = (operator_t*)variant_get_ptr(stack_front);
        if((op_token >= stacked_operator->type || (OP_UNARY_MINUS == op_token && op_token > stacked_operator->type)) 
           && stack_front->type != T_PARETHESIS)
        {
            stack_front = stack_pop_front(operator_stack);
            stack_push_back(operand_queue, stack_front);
            stack_front = stack_peek_front(operator_stack);
        }
        else
        {
            break;
        }
    }

    operator_t* op = operator_create(op_token, NULL);
    stack_push_front(operator_stack, variant_create_ptr(T_OPERATOR, op, NULL));
}

/* 
If the token is a function argument separator (e.g., a comma):
Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue. 
If no left parentheses are encountered, either the separator was misplaced or parentheses were mismatched. 
*/ 
bool process_comma_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue)
{
    variant_t* stack_front = stack_peek_front(operator_stack);
    bool    left_parethesis_found = false;
    while(NULL != stack_front)
    {
        operator_t* stacked_operator = (operator_t*)variant_get_ptr(stack_front);
        if(stacked_operator->type != OP_LEFT_PARETHESIS)
        {
            stack_push_back(operand_queue, stack_pop_front(operator_stack));
        }
        else
        {
            left_parethesis_found = true;
            break;
        }

        stack_front = stack_peek_front(operator_stack);
    }

    return left_parethesis_found;
}

/*
If the token is a left parenthesis (i.e. "("), then push it onto the stack.
If the token is a right parenthesis (i.e. ")"):
Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
Pop the left parenthesis from the stack, but not onto the output queue.
If the token at the top of the stack is a function token, pop it onto the output queue.
If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
*/
bool process_parenthesis_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token)
{
    bool left_parethesis_found = false;

    if(op_token == OP_LEFT_PARETHESIS)
    {
        operator_t* op = operator_create(op_token, NULL);
        stack_push_front(operator_stack, variant_create_ptr(T_PARETHESIS, (void*)op, NULL));
        left_parethesis_found = true;
    }
    else if(op_token == OP_RIGHT_PARETHESIS)
    {
        variant_t* front_data = stack_peek_front(operator_stack);
        
        while(NULL != front_data && left_parethesis_found == false)
        {
            if(front_data->type == T_PARETHESIS && ((operator_t*)variant_get_ptr(front_data))->type == OP_LEFT_PARETHESIS)
            {
                left_parethesis_found = true;
                front_data = stack_pop_front(operator_stack);
                variant_free(front_data);

                front_data = stack_peek_front(operator_stack);

                if(NULL != front_data && front_data->type == T_FUNCTION)
                {
                    front_data = stack_pop_front(operator_stack);
                    stack_push_back(operand_queue, front_data);
                }

                //break;
            }
            else
            {   
                front_data = stack_pop_front(operator_stack);
                stack_push_back(operand_queue, front_data);
                front_data = stack_peek_front(operator_stack);   
            }
        }
    }

    if(!left_parethesis_found)
    {
        LOG_ERROR(Parser, "Parenthesis mismatch");
    }

    return left_parethesis_found;
}

/*
Some rules regarding operand parsing: No spaces or reserved characters (operators) 
are allowed. 
Operand should always start with letter 
There can be two types of string operands: device path or reserved word. 
device paths are always "." separated therefore the dot character is considered always 
to be a delimiter. 
Strings that do not have "." delimiters are considered reserved words and are matched against 
reserved word list. 
*/
bool process_string_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue)
{
    bool isReservedWord = false;
    reserved_word_t* reserved_word = reserved_words;
    bool retVal = true;

    while(*reserved_word->word != '\0' && !isReservedWord)
    {
        if(strcmp(ch, reserved_word->word) == 0)
        {
            variant_t* operand_token = variant_create_bool(reserved_word->value);
            stack_push_back(operand_queue, variant_create_variant(T_OPERAND, operand_token));
            isReservedWord = true;
        }

        *reserved_word++;
    }

    if(!isReservedWord)
    {
        // Now lets tokenize the string and extract data to build an operant_t structure
        char* function_string = strdup((char*)ch);
        //char* tok = strtok((char*)ch, ".");

        char* tok = strrchr(ch, '.');

        if(NULL == tok)
        {
            //retVal = false;
            process_string_operand(ch, operand_queue);
        }
        else
        {
            // Take first part...
            *tok = 0;
            if(resolver_has_name(ch))
            {
                device_record_t* record = resolver_get_device_record(ch);
                
                if(record->devtype == VDEV)
                {
                    retVal = process_vdev_method_operator(ch, tok+1, operator_stack);
                }
                else
                {
                    retVal = process_device_function_operator(ch, tok+1, operator_stack);
                }
            }
            else if(service_manager_is_class_exists(ch))
            {
                retVal = process_service_method_operator(ch, tok+1, operator_stack);
            }
            else if(builtin_service_manager_is_class_exists(ch))
            {
                retVal = process_builtin_method_operator(ch, tok+1, operator_stack);
            }
            else if(vdev_manager_is_vdev_exists(ch))
            {
                retVal = process_vdev_method_operator(ch, tok+1, operator_stack);
            }
            else
            {
                // Well, this might be the only token in expression, save it as string
                // restore string:
                *tok = '.';
                process_string_operand(ch, operand_queue);
            }
        }

        free(function_string);
    }

    return retVal;
}

void process_string_operand(const char* ch, variant_stack_t* operand_queue)
{
    char* arg = strdup(ch);
    variant_t* operand_token = variant_create(DT_STRING, (void*)arg);
    stack_push_back(operand_queue, variant_create_variant(T_OPERAND, operand_token));
}

bool process_device_function_operator(const char* device, const char* method, variant_stack_t* operator_stack)
{
    function_operator_data_t* op_data = (function_operator_data_t*)calloc(1, sizeof(function_operator_data_t));
    operator_t* function_operator = operator_create(OP_DEVICE_FUNCTION, op_data);
    bool    retVal = true;

    device_record_t*    record = resolver_get_device_record(device);
    if(NULL != record)
    {
        op_data->device_record = record;
        op_data->command_class = get_command_class_by_id(record->commandId);
        strncpy(op_data->command_method, method, MAX_METHOD_LEN-1);
        stack_push_front(operator_stack, variant_create_ptr(T_FUNCTION, function_operator, &operator_delete_function_operator));
    }
    else 
    {
        // Error!
        LOG_ERROR(Parser, "Unresolved device %s", device);
        retVal = false;
    }
    return retVal;
}

bool process_vdev_method_operator(const char* device, const char* method, variant_stack_t* operator_stack)
{
    function_operator_data_t* op_data = (function_operator_data_t*)calloc(1, sizeof(function_operator_data_t));
    operator_t* function_operator = operator_create(OP_DEVICE_FUNCTION, op_data);
    bool    retVal = true;

    device_record_t*    record = vdev_manager_create_device_record(device);
    if(NULL != record)
    {
        op_data->device_record = record;
        op_data->command_class = vdev_manager_get_command_class(record->nodeId);
        strncpy(op_data->command_method, method, MAX_METHOD_LEN-1);
        stack_push_front(operator_stack, variant_create_ptr(T_FUNCTION, function_operator, &operator_delete_function_operator));

    }
    else 
    {
        // Error!
        LOG_ERROR(Parser, "Unresolved virtual device %s", device);
        retVal = false;
    }
    return retVal;
}

bool  process_service_method_operator(const char* class, const char* method, variant_stack_t* operator_stack)
{
    bool    retVal = true;

    service_method_t* service_method = service_manager_get_method(class, method);

    if(NULL != service_method)
    {
        operator_t* service_method_operator = operator_create(OP_SERVICE_METHOD, service_method);
        stack_push_front(operator_stack, variant_create_ptr(T_FUNCTION, service_method_operator, &operator_delete_service_method_operator));
    }
    else
    {
        LOG_ERROR(Parser, "Method not defined: %s.%s", class, method);
        retVal = false;
    }

    return retVal;
}

bool process_builtin_method_operator(const char* class, const char* method, variant_stack_t* operator_stack)
{
    bool retVal = true;
    service_method_t* service_method = builtin_service_manager_get_method(class, method);

    if(NULL != service_method)
    {
        operator_t* service_method_operator = operator_create(OP_SERVICE_METHOD, service_method);
        stack_push_front(operator_stack, variant_create_ptr(T_FUNCTION, service_method_operator, &operator_delete_service_method_operator));
    }
    else
    {
        LOG_ERROR(Parser, "Method not defined: %s.%s", class, method);
        retVal = false;
    }

    return retVal;
}


bool process_digit_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue)
{
    errno = 0;
    bool retVal = true;

    char* endptr;
    long double val = strtold(ch, &endptr);

    if(*endptr != 0)
    {
        // Its not a real number!
        retVal = false;
    }
    else
    {
        variant_t* number_variant;
    
        if(val == (int)val)
        {
            number_variant = variant_create_int32(DT_INT32, (int)val);
        }
        else
        {
            number_variant = variant_create_float(val);
        }
    
        stack_push_back(operand_queue, variant_create_variant(T_OPERAND, number_variant));
    }

    return retVal;
}

/*
    Process compiled expression in RPN and calculate the result
    
*/
variant_t*      command_parser_execute_expression(variant_stack_t* compiled_expression)
{
    variant_stack_t*    work_stack = stack_create();
    bool retVal = true;
    variant_t* result = NULL;
    int i = 0;

    while(i < compiled_expression->count && retVal)
    {
        
        variant_t* data = stack_peek_at(compiled_expression, i++); //stack_pop_front(compiled_expression);

        switch(data->type)
        {
        case T_OPERATOR:
        case T_FUNCTION:
            retVal = eval(work_stack, data);
            break;
        default:
            // First time the variant will be deleted during eval(), second time 
            // when we delete compiled_expression (outside of this method)
            stack_push_front(work_stack, variant_clone(variant_get_variant(data)));
            break;
        }
    }

    // At this point there should be only one value in the work stack.
    if(retVal && work_stack->count == 1)
    {
        result = stack_pop_front(work_stack);
    }
    else
    {
        LOG_ERROR(Parser, "Error executing expression");
    }

    stack_free(work_stack);
    return result;
}

bool eval(variant_stack_t* work_stack, variant_t* op)
{
    bool retVal = true;
    operator_t* operator = (operator_t*)variant_get_ptr(op);

    int arg_count = operator->argument_count(operator);
    variant_t* result = NULL;

    if(arg_count > work_stack->count)
    {
        LOG_ERROR(Parser, "Parameter mismatch: required %d, given %d", arg_count, work_stack->count);
        retVal = false;
    }
    else if(arg_count == 0)
    {
        result = operator->eval_callback(operator);
    }
    else if(arg_count == 1)
    {
        variant_t* arg1 = stack_pop_front(work_stack);
        result = operator->eval_callback(operator, arg1);
        variant_free(arg1);
    }
    else if(arg_count == 2)
    {
        variant_t* arg1 = stack_pop_front(work_stack);
        variant_t* arg2 = stack_pop_front(work_stack);

        result = operator->eval_callback(operator, arg2, arg1);

        variant_free(arg1);
        variant_free(arg2);
    }
    else if(arg_count == 3)
    {
        variant_t* arg1 = stack_pop_front(work_stack);
        variant_t* arg2 = stack_pop_front(work_stack);
        variant_t* arg3 = stack_pop_front(work_stack);

        result = operator->eval_callback(operator, arg3, arg2, arg1);
        variant_free(arg1);
        variant_free(arg2);
        variant_free(arg3);
    }
    else if(arg_count == 4)
    {
        variant_t* arg1 = stack_pop_front(work_stack);
        variant_t* arg2 = stack_pop_front(work_stack);
        variant_t* arg3 = stack_pop_front(work_stack);
        variant_t* arg4 = stack_pop_front(work_stack);

        result = operator->eval_callback(operator, arg4, arg3, arg2, arg1);
        variant_free(arg1);
        variant_free(arg2);
        variant_free(arg3);
        variant_free(arg4);
    }
    else
    {
        retVal = false;
    }

    if(NULL != result)
    {
        stack_push_front(work_stack, result);
    }

    return retVal;
}

void        command_parser_print_stack(variant_stack_t* compiled_expression)
{
    stack_item_t* stack_item = compiled_expression->head;

    while(NULL != stack_item)
    {
        variant_t* data = stack_item->data;
        
        variant_t* value = (variant_t*)variant_get_ptr(data);

        switch(data->type)
        {
        case T_OPERAND:
            LOG_DEBUG(Parser, " %d ", variant_get_int(value));
            break;
        case T_OPERATOR:
            {
                operator_t* op = (operator_t*)value;
                LOG_DEBUG(Parser, " oper %d ", op->type);
            }
            break;
        case T_FUNCTION:
            {
                operator_t* op = (operator_t*)value;
                LOG_DEBUG(Parser, " func %s ", ((function_operator_data_t*)(op->operator_data))->device_record->deviceName);
            }
            break;
        }

        stack_item = stack_item->next;
    }

    LOG_DEBUG(Parser, "\r\n");
}
