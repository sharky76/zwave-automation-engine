/*
Alphabet:
        [A-Za-z] [0-9] < > ) ( , && || == EOL SPACE


States:
        start
        alpha
        digit
        left parenthesis
        right parenthesis
        comma
        AND
        OR
        CMP
        LESS
        MORE
        error
        end

Transitions:
        
        start -> [A-Za-z] -> alpha
        start -> [0-9] -> digit
        start -> ) -> right parenthesis
        start -> ( -> left parenthesis
        start -> , -> comma
        start -> && -> AND
        start -> || -> OR
        start -> == -> CMP
        start -> EOL -> end
        start -> SPACE -> start
        
        alpha -> [A-Za-z] -> alpha
        alpha -> [0-9] -> alpha
        alpha -> ) -> right parenthesis
        alpha -> ( -> left parenthesis
        alpha -> , -> comma
        alpha -> && -> AND
        alpha -> || -> OR
        alpha -> == -> CMP
        alpha -> EOL -> end
        alpha -> SPACE -> start

        digit -> [A-Za-z] -> error
        digit -> [0-9] -> digit
        digit -> ) -> right parenthesis
        digit -> ( -> error
        digit -> , -> comma
        digit -> && -> AND
        digit -> || -> OR
        digit -> == -> CMP
        digit -> EOL -> end
        digit -> SPACE -> start

        left parenthesis -> [A-Za-z] -> alpha
        left parenthesis -> [0-9] -> digit
        left parenthesis -> ) -> right parenthesis
        left parenthesis -> ( -> left parenthesis
        left parenthesis -> , -> error
        left parenthesis -> && -> error
        left parenthesis -> || -> error
        left parenthesis -> == -> error
        left parenthesis -> EOL -> error
        left parenthesis -> SPACE -> start
 
        right parenthesis -> [A-Za-z] -> error
        right parenthesis -> [0-9] -> error
        right parenthesis -> ) -> right parenthesis
        right parenthesis -> ( -> error
        right parenthesis -> , -> comma
        right parenthesis -> && -> AND
        right parenthesis -> || -> OR
        right parenthesis -> == -> CMP
        right parenthesis -> EOL -> end
        right parenthesis -> SPACE -> start
 
        comma -> [A-Za-z] -> alpha
        comma -> [0-9] -> digit
        comma -> ) -> error
        comma -> ( -> left parenthesis
        comma -> , -> error
        comma -> && -> error
        comma -> || -> error
        comma -> == -> error
        comma -> EOL -> error
        comma -> SPACE -> start
 
        AND -> [A-Za-z] -> alpha
        AND -> [0-9] -> digit
        AND -> ) -> error
        AND -> ( -> left parenthesis
        AND -> , -> error
        AND -> && -> error
        AND -> || -> error
        AND -> == -> error
        AND -> EOL -> error
        AND -> SPACE -> start
 
        OR -> [A-Za-z] -> alpha
        OR -> [0-9] -> digit
        OR -> ) -> error
        OR -> ( -> left parenthesis
        OR -> , -> error
        OR -> && -> error
        OR -> || -> error
        OR -> == -> error
        OR -> EOL -> error
        OR -> SPACE -> start

        CMP -> [A-Za-z] -> alpha
        CMP -> [0-9] -> digit
        CMP -> ) -> error
        CMP -> ( -> left parenthesis
        CMP -> , -> error
        CMP -> && -> error
        CMP -> || -> error
        CMP -> == -> error 
        CMP -> EOL -> error
        CMP -> SPACE -> start
 
        LESS -> [A-Za-z] -> alpha
        LESS -> [0-9] -> digit
        LESS -> ) -> error
        LESS -> ( -> left parenthesis
        LESS -> , -> error
        LESS -> && -> error
        LESS -> || -> error
        LESS -> == -> error 
        LESS -> EOL -> error
        LESS -> SPACE -> start
 
        MORE -> [A-Za-z] -> alpha
        MORE -> [0-9] -> digit
        MORE -> ) -> error
        MORE -> ( -> left parenthesis
        MORE -> , -> error
        MORE -> && -> error
        MORE -> || -> error
        MORE -> == -> error 
        MORE -> EOL -> error
        MORE -> SPACE -> start
*/
#include <stdbool.h>

#define MAX_TOKEN_LEN   2

typedef enum AlphabetToken_e
{
    A_ALPHA,
    A_DIGIT,
    A_RIGHT_PAREN,
    A_LEFT_PAREN,
    A_COMMA,
    A_AND,
    A_OR,
    A_CMP,
    A_LESS,
    A_MORE,
    A_EOL,
    A_SPACE,
    A_QUOTE,
    A_DOT,
    A_PLUS,
    A_MINUS,
    A_SLASH,
    A_INVALID,
} AlphabetToken;

typedef enum State_e
{
    STATE_START,
    STATE_ALPHA,
    STATE_DIGIT,
    STATE_RIGHT_PAREN,
    STATE_LEFT_PAREN,
    STATE_COMMA,
    STATE_AND,
    STATE_OR,
    STATE_CMP,
    STATE_LESS,
    STATE_MORE,
    STATE_PLUS,
    STATE_MINUS,
    STATE_CAPTURE_STRING,
    STATE_ERROR,
    STATE_END,
    STATE_INVALID
} State;

typedef struct dfa_transition_t
{
    State           source_state;
    AlphabetToken   token;
    State           target_state;
} dfa_transition_t;

static dfa_transition_t    dfa_transitions[];

typedef enum TokenParsingState_e
{
    PS_NONE,
    PS_STARTED,
    PS_COMPLETED,
    PS_ERROR
} TokenParsingState;

typedef struct state_context_t
{
    const char* expression;
    int         pos;
    State           current_state;
    AlphabetToken   current_token;
    char            current_data_buf[256];

    TokenParsingState   token_parsing_state;
    char                parsed_token[MAX_TOKEN_LEN];
    int                 parse_cycle;

} state_context_t;

typedef struct state_descriptor_t
{
    State   state;
    State   (*state_callback)(state_context_t*, void*);
} state_descriptor_t;

State           parser_dfa_next_state(State source_state, AlphabetToken token);
AlphabetToken   parser_dfa_get_token(const char* str, State source_state);
AlphabetToken   parser_dfa_read_next_token(state_context_t* state_context);

bool            parser_dfa_run(State start_state, State end_state, State error_state, state_descriptor_t state_map[], int map_size, const char* expression, void* priv);
