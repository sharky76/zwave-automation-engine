#include "parser_dfa.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static dfa_transition_t    dfa_transitions[] = {
    // From start
    {STATE_START,   A_ALPHA,       STATE_ALPHA},
    {STATE_START,   A_DIGIT,       STATE_DIGIT},
    {STATE_START,   A_RIGHT_PAREN, STATE_RIGHT_PAREN},
    {STATE_START,   A_LEFT_PAREN,  STATE_LEFT_PAREN},
    {STATE_START,   A_COMMA,       STATE_COMMA},
    {STATE_START,   A_AND,         STATE_AND},
    {STATE_START,   A_OR,          STATE_OR},
    {STATE_START,   A_PLUS,        STATE_PLUS},
    {STATE_START,   A_MINUS,       STATE_MINUS},
    {STATE_START,   A_CMP,         STATE_CMP},
    {STATE_START,   A_LESS,        STATE_LESS},
    {STATE_START,   A_MORE,        STATE_MORE},
    {STATE_START,   A_EOL,         STATE_END},
    {STATE_START,   A_SPACE,       STATE_START},
    {STATE_START,   A_QUOTE,       STATE_CAPTURE_STRING},

    // From alpha
    {STATE_ALPHA,   A_ALPHA,        STATE_ALPHA},
    {STATE_ALPHA,   A_DIGIT,        STATE_ALPHA},
    {STATE_ALPHA,   A_RIGHT_PAREN,  STATE_RIGHT_PAREN},
    {STATE_ALPHA,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_ALPHA,   A_COMMA,        STATE_COMMA},
    {STATE_ALPHA,   A_AND,          STATE_AND},
    {STATE_ALPHA,   A_OR,           STATE_OR},
    {STATE_ALPHA,   A_PLUS,         STATE_PLUS},
    {STATE_ALPHA,   A_MINUS,        STATE_MINUS},
    {STATE_ALPHA,   A_CMP,          STATE_CMP},
    {STATE_ALPHA,   A_LESS,         STATE_LESS},
    {STATE_ALPHA,   A_MORE,         STATE_MORE},
    {STATE_ALPHA,   A_DOT,          STATE_ALPHA},
    {STATE_ALPHA,   A_EOL,          STATE_END},
    {STATE_ALPHA,   A_SPACE,        STATE_ALPHA},

    // From digit
    {STATE_DIGIT,   A_DIGIT,        STATE_DIGIT},
    {STATE_DIGIT,   A_RIGHT_PAREN,  STATE_RIGHT_PAREN},
    {STATE_DIGIT,   A_COMMA,        STATE_COMMA},
    {STATE_DIGIT,   A_AND,          STATE_AND},
    {STATE_DIGIT,   A_OR,           STATE_OR},
    {STATE_DIGIT,   A_PLUS,         STATE_PLUS},
    {STATE_DIGIT,   A_MINUS,        STATE_MINUS},
    {STATE_DIGIT,   A_CMP,          STATE_CMP},
    {STATE_DIGIT,   A_LESS,         STATE_LESS},
    {STATE_DIGIT,   A_MORE,         STATE_MORE},
    {STATE_DIGIT,   A_DOT,          STATE_DIGIT},
    {STATE_DIGIT,   A_EOL,          STATE_END},
    {STATE_DIGIT,   A_SPACE,        STATE_START},

    // From left parenthesis
    {STATE_LEFT_PAREN,   A_ALPHA,        STATE_ALPHA},
    {STATE_LEFT_PAREN,   A_DIGIT,        STATE_DIGIT},
    {STATE_LEFT_PAREN,   A_RIGHT_PAREN,  STATE_RIGHT_PAREN},
    {STATE_LEFT_PAREN,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_LEFT_PAREN,   A_SPACE,        STATE_START},
    {STATE_LEFT_PAREN,   A_QUOTE,        STATE_CAPTURE_STRING},

    // From right parenthesis
    {STATE_RIGHT_PAREN,   A_RIGHT_PAREN,  STATE_RIGHT_PAREN},
    {STATE_RIGHT_PAREN,   A_COMMA,        STATE_COMMA},
    {STATE_RIGHT_PAREN,   A_AND,          STATE_AND},
    {STATE_RIGHT_PAREN,   A_OR,           STATE_OR},
    {STATE_RIGHT_PAREN,   A_PLUS,         STATE_PLUS},
    {STATE_RIGHT_PAREN,   A_MINUS,        STATE_MINUS},
    {STATE_RIGHT_PAREN,   A_CMP,          STATE_CMP},
    {STATE_RIGHT_PAREN,   A_LESS,         STATE_LESS},
    {STATE_RIGHT_PAREN,   A_MORE,         STATE_MORE},
    {STATE_RIGHT_PAREN,   A_EOL,          STATE_END},
    {STATE_RIGHT_PAREN,   A_SPACE,        STATE_START},

    // From comma
    {STATE_COMMA,   A_ALPHA,        STATE_ALPHA},
    {STATE_COMMA,   A_DIGIT,        STATE_DIGIT},
    {STATE_COMMA,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_COMMA,   A_SPACE,        STATE_START},

    // From AND
    {STATE_AND,   A_ALPHA,        STATE_ALPHA},
    {STATE_AND,   A_DIGIT,        STATE_DIGIT},
    {STATE_AND,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_AND,   A_SPACE,        STATE_START},
    
    // From OR
    {STATE_OR,   A_ALPHA,        STATE_ALPHA},
    {STATE_OR,   A_DIGIT,        STATE_DIGIT},
    {STATE_OR,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_OR,   A_SPACE,        STATE_START},

    // From PLUS
    {STATE_PLUS, A_ALPHA,        STATE_ALPHA},
    {STATE_PLUS, A_DIGIT,        STATE_DIGIT},
    {STATE_PLUS, A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_PLUS, A_SPACE,        STATE_START},

    // From MINUS
    {STATE_MINUS, A_ALPHA,        STATE_ALPHA},
    {STATE_MINUS, A_DIGIT,        STATE_DIGIT},
    {STATE_MINUS, A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_MINUS, A_SPACE,        STATE_START},

    // From CMP
    {STATE_CMP,   A_ALPHA,        STATE_ALPHA},
    {STATE_CMP,   A_DIGIT,        STATE_DIGIT},
    {STATE_CMP,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_CMP,   A_SPACE,        STATE_START},

    // From LESS
    {STATE_LESS,   A_ALPHA,        STATE_ALPHA},
    {STATE_LESS,   A_DIGIT,        STATE_DIGIT},
    {STATE_LESS,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_LESS,   A_SPACE,        STATE_START},

    // From MORE
    {STATE_MORE,   A_ALPHA,        STATE_ALPHA},
    {STATE_MORE,   A_DIGIT,        STATE_DIGIT},
    {STATE_MORE,   A_LEFT_PAREN,   STATE_LEFT_PAREN},
    {STATE_MORE,   A_SPACE,        STATE_START},

    // Quote
    {STATE_CAPTURE_STRING,   A_ALPHA,        STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_DIGIT,        STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_RIGHT_PAREN,  STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_LEFT_PAREN,   STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_COMMA,        STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_AND,          STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_OR,           STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_PLUS,         STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_MINUS,        STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_CMP,          STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_LESS,         STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_MORE,         STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_DOT,          STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_EOL,          STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_SPACE,        STATE_CAPTURE_STRING},
    {STATE_CAPTURE_STRING,   A_QUOTE,        STATE_START},  

    {STATE_ERROR,   A_ALPHA,        STATE_END},
    {STATE_ERROR,   A_DIGIT,        STATE_END},
    {STATE_ERROR,   A_RIGHT_PAREN,  STATE_END},
    {STATE_ERROR,   A_LEFT_PAREN,   STATE_END},
    {STATE_ERROR,   A_COMMA,        STATE_END},
    {STATE_ERROR,   A_AND,          STATE_END},
    {STATE_ERROR,   A_OR,           STATE_END},
    {STATE_ERROR,   A_PLUS,         STATE_END},
    {STATE_ERROR,   A_MINUS,        STATE_END},
    {STATE_ERROR,   A_CMP,          STATE_END},
    {STATE_ERROR,   A_LESS,         STATE_END},
    {STATE_ERROR,   A_MORE,         STATE_END},
    {STATE_ERROR,   A_DOT,          STATE_END},
    {STATE_ERROR,   A_EOL,          STATE_END},
    {STATE_ERROR,   A_SPACE,        STATE_END},
    {STATE_ERROR,   A_QUOTE,        STATE_END},            
};

State           parser_dfa_next_state(State source_state, AlphabetToken token)
{
    int i;
    State target_state = STATE_INVALID;

    if(A_INVALID == token)
    {
        return target_state;
    }

    for(i = 0; i < sizeof(dfa_transitions) / sizeof(dfa_transition_t) && target_state == STATE_INVALID; i++ )
    {
        if(dfa_transitions[i].source_state == source_state && dfa_transitions[i].token == token)
        {
            target_state = dfa_transitions[i].target_state;
        }
    }

    return target_state;
}

AlphabetToken   parser_dfa_get_token(const char* str, State source_state)
{
    if(isalpha(*str) || *str == '$' || *str == ':')
    {
        return A_ALPHA;
    }
    else if(*str == '.')
    {
        return A_DOT;
    }
    else if(isdigit(*str))
    {
        return A_DIGIT;
    }
    else if(isspace(*str))
    {
        return A_SPACE;
    }
    else if(*str == ')')
    {
        return A_RIGHT_PAREN;
    }
    else if(*str == '(')
    {
        return A_LEFT_PAREN;
    }
    else if(*str == ',')
    {
        return A_COMMA;
    }
    else if(*str == '\0' || *str == '\n' || *str == '\r')
    {
        return A_EOL;
    }
    else if(*str == '<')
    {
        return A_LESS;
    }
    else if(*str == '>')
    {
        return A_MORE;
    }
    else if(*str == '\'')
    {
        return A_QUOTE;
    }
    else if(*str == '+')
    {
        return A_PLUS;
    }
    else if(*str == '-')
    {
        return A_MINUS;
    }
    else if(strlen(str) == 2)
    {
        if(strcmp(str, "&&") == 0)
        {
            return A_AND;
        }
        else if(strcmp(str, "||") == 0)
        {
            return A_OR;
        }
        else if(strcmp(str, "==") == 0)
        {
            return A_CMP;
        }
        else if(strcmp(str, "\\\'") == 0)
        {
            return A_ALPHA;
        }
    }
    else if(*str == '\\')
    {
        return A_INVALID;
    }

    if(source_state == STATE_CAPTURE_STRING)
    {
        return A_ALPHA;
    }

    return A_INVALID;
}

AlphabetToken   parser_dfa_read_next_token(state_context_t* state_context)
{
    //state_context->pos++;

    char ch = state_context->expression[state_context->pos++];

    if(state_context->token_parsing_state == PS_NONE)
    {
        memset(state_context->parsed_token, 0, MAX_TOKEN_LEN);

        state_context->token_parsing_state = PS_STARTED;
        state_context->parse_cycle = 0;
        state_context->parsed_token[state_context->parse_cycle++] = ch;
    }
    else if(state_context->token_parsing_state == PS_STARTED)
    {
        state_context->parsed_token[state_context->parse_cycle++] = ch;
    }
    else if(state_context->token_parsing_state == PS_ERROR)
    {
        return A_INVALID;
    }

    AlphabetToken token = parser_dfa_get_token(state_context->parsed_token, state_context->current_state);
    if(A_INVALID == token)
    {
        if(state_context->parse_cycle <= MAX_TOKEN_LEN)
        {
            return parser_dfa_read_next_token(state_context);
        }
        else
        {
            state_context->token_parsing_state = PS_ERROR;
            state_context->current_token = A_INVALID;
        }
    }
    else
    {
        state_context->token_parsing_state = PS_COMPLETED;
        state_context->current_token = token;
        /*if(token == A_ALPHA || token == A_DIGIT || token == A_DOT)
        {
            strncat(state_context->current_data_buf, state_context->parsed_token, state_context->parse_cycle);
        }*/
        //*state_context->parsed_token = '\0';
    }

    return state_context->current_token;
}

state_descriptor_t* parser_dfa_find_descriptor(State state, state_descriptor_t state_map[], int map_size)
{
    state_descriptor_t* descr = NULL;
    int i;

    // Find the state
    for(i = 0; i < map_size && descr == NULL; i++)
    {
        if(state_map[i].state == state)
        {
            descr = &state_map[i];
        }
    }

    return descr;
}

bool           parser_dfa_run(State start_state, State end_state, State error_state, state_descriptor_t state_map[], int map_size, const char* expression, void* priv)
{
    int i;
    State current_state = start_state;
    bool retVal;

    state_descriptor_t* state_descriptor = parser_dfa_find_descriptor(start_state, state_map, map_size);
    state_descriptor_t* end_descriptor = parser_dfa_find_descriptor(end_state, state_map, map_size);
    state_descriptor_t* error_descriptor = parser_dfa_find_descriptor(error_state, state_map, map_size);

    state_context_t state_context = {0};

    state_context.expression = expression;
    state_context.current_state = start_state;

    while(current_state != end_state && current_state != error_state)
    {
        current_state = state_descriptor->state_callback(&state_context, priv);
        state_context.token_parsing_state = PS_NONE;
        state_descriptor = parser_dfa_find_descriptor(current_state, state_map, map_size);

        if(NULL == state_descriptor)
        {
            state_descriptor = error_descriptor;
        }
    }

    switch(current_state)
    {
    case STATE_ERROR:
        error_descriptor->state_callback(&state_context, priv);
        retVal = false;
        break;
    case STATE_END:
        {
            State final_state = end_descriptor->state_callback(&state_context, priv);
            retVal = (final_state != STATE_ERROR);
        }
        break;
    }

    return retVal;
}
