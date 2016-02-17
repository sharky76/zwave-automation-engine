#include "command_parser.h"
#include "resolver.h"
#include "parser_dfa.h"
#include "service_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <logger.h>

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

void process_string_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue);
void process_string_operand(const char* ch, variant_stack_t* operand_queue);
void process_service_method_operator(const char* ch, variant_stack_t* operator_stack);
void process_device_function_operator(const char* ch, variant_stack_t* operator_stack);
void process_digit_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue);

void process_operator_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token);
int process_parenthesis_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token);

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
State    state_cmp(state_context_t* state_context, void* priv);
State    state_less(state_context_t* state_context, void* priv);
State    state_more(state_context_t* state_context, void* priv);
State    state_capture_string(state_context_t* state_context, void* priv);
State    state_error(state_context_t* state_context, void* priv);
State    state_end(state_context_t* state_context, void* priv);
State    state_invalid(state_context_t* state_context, void* priv);

typedef struct parser_data_t
{
    variant_stack_t* operator_stack;
    variant_stack_t* operand_queue;
} parser_data_t;

void    process_state(State current_state, state_context_t* state_context, void* priv);

static state_descriptor_t state_map[] = {
    {STATE_START,   &state_start},
    {STATE_ALPHA,   &state_alpha},
    {STATE_DIGIT,   &state_digit},
    {STATE_RIGHT_PAREN, &state_right_paren},
    {STATE_LEFT_PAREN,  &state_left_paren},
    {STATE_COMMA,   &state_comma},
    {STATE_AND,     &state_and},
    {STATE_OR,      &state_or},
    {STATE_CMP,     &state_cmp},
    {STATE_LESS,    &state_less},
    {STATE_MORE,    &state_more},
    {STATE_CAPTURE_STRING,  &state_capture_string},
    {STATE_ERROR,   &state_error},
    {STATE_END,     &state_end},
    {STATE_INVALID, &state_invalid}
};

void process_state(State current_state, state_context_t* state_context, void* priv)
{
    parser_data_t* parser_data = (parser_data_t*)priv;
    if(state_context->current_state != current_state)
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
            process_string_token(state_context->current_data_buf, parser_data->operator_stack, parser_data->operand_queue);
            *state_context->current_data_buf = 0;
            break;
        case STATE_DIGIT:
            process_digit_token(state_context->current_data_buf, parser_data->operator_stack, parser_data->operand_queue);
            *state_context->current_data_buf = 0;
            break;
        case STATE_RIGHT_PAREN:
            process_parenthesis_token(parser_data->operator_stack, parser_data->operand_queue, OP_RIGHT_PARETHESIS);
            break;
        case STATE_LEFT_PAREN:
            process_parenthesis_token(parser_data->operator_stack, parser_data->operand_queue, OP_LEFT_PARETHESIS);
            break;
        case STATE_COMMA:
            // No support for comma right now
            break;
        case STATE_AND:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_AND);
            break;
        case STATE_OR:
            process_operator_token(parser_data->operator_stack, parser_data->operand_queue, OP_OR);
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
            break;
        case STATE_START:
            break;
        case STATE_ERROR:
            break;
        default:
            printf("Error, invalid transition\n");
        }
    }
}

State    state_start(state_context_t* state_context, void* priv)
{
    process_state(STATE_START, state_context, priv);
    state_context->current_state = STATE_START;
    return parser_dfa_next_state(STATE_START, parser_dfa_read_next_token(state_context));
}

State    state_alpha(state_context_t* state_context, void* priv)
{
    process_state(STATE_ALPHA, state_context, priv);
    state_context->current_state = STATE_ALPHA;
    return parser_dfa_next_state(STATE_ALPHA, parser_dfa_read_next_token(state_context));
}

State    state_digit(state_context_t* state_context, void* priv)
{
    process_state(STATE_DIGIT, state_context, priv);
    state_context->current_state = STATE_DIGIT;
    return parser_dfa_next_state(STATE_DIGIT, parser_dfa_read_next_token(state_context));
}

State    state_right_paren(state_context_t* state_context, void* priv)
{
    process_state(STATE_RIGHT_PAREN, state_context, priv);
    state_context->current_state = STATE_RIGHT_PAREN;
    return parser_dfa_next_state(STATE_RIGHT_PAREN, parser_dfa_read_next_token(state_context));
}

State    state_left_paren(state_context_t* state_context, void* priv)
{
    process_state(STATE_LEFT_PAREN, state_context, priv);
    state_context->current_state = STATE_LEFT_PAREN;
    return parser_dfa_next_state(STATE_LEFT_PAREN, parser_dfa_read_next_token(state_context));
}

State    state_comma(state_context_t* state_context, void* priv)
{
    process_state(STATE_COMMA, state_context, priv);
    state_context->current_state = STATE_COMMA;
    return parser_dfa_next_state(STATE_COMMA, parser_dfa_read_next_token(state_context));
}

State    state_and(state_context_t* state_context, void* priv)
{
    process_state(STATE_AND, state_context, priv);
    state_context->current_state = STATE_AND;
    return parser_dfa_next_state(STATE_AND, parser_dfa_read_next_token(state_context));
}

State    state_or(state_context_t* state_context, void* priv)
{
    process_state(STATE_OR, state_context, priv);
    state_context->current_state = STATE_OR;
    return parser_dfa_next_state(STATE_OR, parser_dfa_read_next_token(state_context));
}

State    state_cmp(state_context_t* state_context, void* priv)
{
    process_state(STATE_CMP, state_context, priv);
    state_context->current_state = STATE_CMP;
    return parser_dfa_next_state(STATE_CMP, parser_dfa_read_next_token(state_context));
}

State    state_less(state_context_t* state_context, void* priv)
{
    process_state(STATE_LESS, state_context, priv);
    state_context->current_state = STATE_LESS;
    return parser_dfa_next_state(STATE_LESS, parser_dfa_read_next_token(state_context));
}

State    state_more(state_context_t* state_context, void* priv)
{
    process_state(STATE_MORE, state_context, priv);
    state_context->current_state = STATE_MORE;
    return parser_dfa_next_state(STATE_MORE, parser_dfa_read_next_token(state_context));
}

State    state_capture_string(state_context_t* state_context, void* priv)
{
    process_state(STATE_CAPTURE_STRING, state_context, priv);
    state_context->current_state = STATE_CAPTURE_STRING;
    return parser_dfa_next_state(STATE_CAPTURE_STRING, parser_dfa_read_next_token(state_context));
}

State    state_error(state_context_t* state_context, void* priv)
{
    LOG_ERROR("Error parsing expression: %s", state_context->expression);
    state_context->current_state = STATE_ERROR;
    return STATE_ERROR;
}

State    state_end(state_context_t* state_context, void* priv)
{
    process_state(STATE_END, state_context, priv);

    parser_data_t* parser_data = (parser_data_t*)priv;
    while(parser_data->operator_stack->count > 0)
    {
        variant_t* data = stack_pop_front(parser_data->operator_stack);
        stack_push_back(parser_data->operand_queue, data);
    }

    state_context->current_state = STATE_END;

    return STATE_END;
}

State    state_invalid(state_context_t* state_context, void* priv)
{
    LOG_ERROR("Invalid token received: %s", state_context->parsed_token);
    state_context->current_state = STATE_INVALID;
    return STATE_ERROR;
}

variant_stack_t*    command_parser_compile_expression(const char* expression, bool* isOk)
{
    variant_stack_t* operand_queue = stack_create();
    variant_stack_t* operator_stack = stack_create();

    LOG_DEBUG("Parsing expression: %s", expression);

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

// NOTE: All our operators are left-associative
void process_operator_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token)
{
    variant_t* stack_front = stack_peek_front(operator_stack);
    while(NULL != stack_front)
    {
        operator_t* stacked_operator = (operator_t*)variant_get_ptr(stack_front);
        if(op_token >= stacked_operator->type)
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
If the token is a left parenthesis (i.e. "("), then push it onto the stack.
If the token is a right parenthesis (i.e. ")"):
Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
Pop the left parenthesis from the stack, but not onto the output queue.
If the token at the top of the stack is a function token, pop it onto the output queue.
If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
*/
int process_parenthesis_token(variant_stack_t* operator_stack, variant_stack_t* operand_queue, OperatorType op_token)
{
    int left_parethesis_found = FALSE;

    if(op_token == OP_LEFT_PARETHESIS)
    {
        operator_t* op = operator_create(op_token, NULL);
        stack_push_front(operator_stack, variant_create_ptr(T_PARETHESIS, (void*)op, NULL));
        left_parethesis_found = TRUE;
    }
    else if(op_token == OP_RIGHT_PARETHESIS)
    {
        variant_t* front_data = stack_peek_front(operator_stack);
        
        while(NULL != front_data && left_parethesis_found == FALSE)
        {
            if(front_data->type == T_PARETHESIS && ((operator_t*)variant_get_ptr(front_data))->type == OP_LEFT_PARETHESIS)
            {
                left_parethesis_found = TRUE;
                front_data = stack_pop_front(operator_stack);
                variant_free(front_data);

                front_data = stack_peek_front(operator_stack);

                if(NULL == front_data)
                {
                    LOG_ERROR("Parenthesis mismatch");
                }
                else if(front_data->type == T_FUNCTION)
                {
                    front_data = stack_pop_front(operator_stack);
                    stack_push_back(operand_queue, front_data);
                }
            }
            else
            {   
                front_data = stack_pop_front(operator_stack);
                stack_push_back(operand_queue, front_data);
            }

            
        }
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
void process_string_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue)
{
    int isReservedWord = FALSE;
    reserved_word_t* reserved_word = reserved_words;

    while(*reserved_word->word != '\0' && !isReservedWord)
    {
        if(strcmp(ch, reserved_word->word) == 0)
        {
            variant_t* operand_token = variant_create_bool(reserved_word->value);
            stack_push_back(operand_queue, variant_create_variant(T_OPERAND, operand_token));
            isReservedWord = TRUE;
        }

        *reserved_word++;
    }

    if(!isReservedWord)
    {
        // Now lets tokenize the string and extract data to build an operant_t structure
        char* function_string = strdup((char*)ch);
        char* tok = strtok((char*)ch, ".");

        if(NULL == tok)
        {
            // Error!
        }
        else
        {
            if(resolver_has_name(tok))
            {
                process_device_function_operator(function_string, operator_stack);
            }
            else if(service_manager_is_class_exists(tok))
            {
                process_service_method_operator(function_string, operator_stack);
            }
            else
            {
                // Well, this might be the only token in expression, save it as string
                process_string_operand(ch, operand_queue);
            }
        }

        free(function_string);
    }
}

void process_string_operand(const char* ch, variant_stack_t* operand_queue)
{
    char* arg = strdup(ch);
    variant_t* operand_token = variant_create(DT_STRING, (void*)arg);
    stack_push_back(operand_queue, variant_create_variant(T_OPERAND, operand_token));
}

void process_device_function_operator(const char* ch, variant_stack_t* operator_stack)
{
    function_operator_data_t* op_data = (function_operator_data_t*)malloc(sizeof(function_operator_data_t));
    operator_t* function_operator = operator_create(OP_DEVICE_FUNCTION, op_data);

    char* function_string = (char*)ch;
    char* tok = strtok(function_string, ".");
    int tok_count = 0;
    while(NULL != tok)
    {
        // First argument is device name
        switch(tok_count++)
        {
        case 0: // Device name
            {
                device_record_t*    record = resolver_get_device_record(tok);
                if(NULL != record)
                {
                    ZWBYTE node_id = record->nodeId;
                    op_data->device_record = record;
                    op_data->instance_id = record->instanceId;
                    op_data->command_class = get_command_class_by_id(record->commandId);
                }
                else 
                {
                    // Error!
                }
            }
            break;
        case 1: // Command and arguments
            if(strstr(tok, "Get") == tok)
            {
                op_data->command_method = M_GET;
            }
            else if(strstr(tok, "Set") == tok)
            {
                op_data->command_method = M_SET;
            }
            break;
        }

        tok = strtok(NULL, ".");
    }

    stack_push_front(operator_stack, variant_create_ptr(T_FUNCTION, function_operator, &operator_delete_function_operator));
}

void  process_service_method_operator(const char* ch, variant_stack_t* operator_stack)
{
    char* function_string = (char*)ch;
    char* tok = strtok(function_string, ".");
    int tok_count = 0;

    char* service_class;
    char* name;

    while(NULL != tok)
    {
        switch(tok_count++)
        {
        case 0:
            service_class = tok;
            break;
        case 1:
            name = tok;
            break;
        }
    
        tok = strtok(NULL, ".");
    }

    service_method_t* service_method = service_manager_get_method(service_class, name);

    if(NULL != service_method)
    {
        operator_t* service_method_operator = operator_create(OP_SERVICE_METHOD, service_method);
        stack_push_front(operator_stack, variant_create_ptr(T_FUNCTION, service_method_operator, &operator_delete_service_method_operator));
    }
}

void process_digit_token(const char* ch, variant_stack_t* operator_stack, variant_stack_t* operand_queue)
{
    errno = 0;
    long double val = strtold(ch, NULL);

    variant_t* number_variant;

    if(val == (int)val)
    {
        number_variant = variant_create_int32(DT_INT32, (int)val);
    }
    else
    {
        number_variant = variant_create_float(val);
    }

    stack_push_back(operand_queue, variant_create_ptr(T_OPERAND, number_variant, NULL));
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
            stack_push_front(work_stack, variant_get_variant(data));
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
        LOG_ERROR("Error executing expression");
    }

    stack_free(work_stack);
    return result;
}

bool eval(variant_stack_t* work_stack, variant_t* op)
{
    bool retVal = false;
    operator_t* operator = (operator_t*)variant_get_ptr(op);

    int arg_count = operator->argument_count(operator->operator_data);
    variant_t* result;

    if(arg_count == 0)
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

    if(NULL != result)
    {
        stack_push_front(work_stack, result);
        retVal = true;
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
            printf(" %d ", variant_get_int(value));
            break;
        case T_OPERATOR:
            {
                operator_t* op = (operator_t*)value;
                printf(" oper %d ", op->type);
            }
            break;
        case T_FUNCTION:
            {
                operator_t* op = (operator_t*)value;
                printf(" func %s ", ((function_operator_data_t*)(op->operator_data))->device_record->deviceName);
            }
            break;
        }

        stack_item = stack_item->next;
    }

    printf("\n");
}
