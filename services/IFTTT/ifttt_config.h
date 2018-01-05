#pragma once

extern char*    ifttt_key;

typedef struct ifttt_action_t
{
    char* url;
    char* handler_type;
    char* handler_name;
} ifttt_action_t;

