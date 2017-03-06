#include "cli.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

int DT_CMD_NODE = DT_USER+1;
int DT_CLI_NODE = DT_USER+2;
    
variant_stack_t*    cli_node_list;
cli_node_t*         root_node;

cli_command_t end_cmd = {
    .name = "end",
    .help = "End current configuration",
    .func = cli_exit_node
};

node_data_t*    cmd_create_node_data(const char* cmd_part, cli_command_t* cmd)
{
    node_data_t* data = calloc(1, sizeof(node_data_t));
    data->has_range = false;
    data->name = strdup(cmd_part);
    //data->is_term = false;
    data->command = cmd;

    if(strcmp(cmd_part, "WORD") == 0)
    {
        data->type = TYPE_WORD;
    }
    else if(strcmp(cmd_part, "LINE") == 0)
    {
        data->type = TYPE_LINE;
    }
    else if(strstr(cmd_part, "INT") == cmd_part)
    {
        data->type = TYPE_INT;
        if(strchr(cmd_part, '[') != 0)
        {
            int start;
            int end;
            if(sscanf(cmd_part, "INT[%d-%d]", &start, &end) == 2)
            {
                data->value_range.start = variant_create_int32(DT_INT32, start);
                data->value_range.end = variant_create_int32(DT_INT32, end);
                data->has_range = true;
            }
        }

    }
    else if(strcmp(cmd_part, "CHAR") == 0)
    {
        data->type = TYPE_CHAR;
    }
    else
    {
        data->type = TYPE_CMD_PART;
    }

    return data;
}

node_data_t*    cmd_create_final_node_data(cli_command_t* cmd)
{
    node_data_t* data = calloc(1, sizeof(node_data_t));
    data->has_range = false;

    data->name = strdup("<cr>");
    //data->is_term = true;
    data->command = cmd;
    data->type = TYPE_TERM;
    return data;
}

void    cmd_tree_node_delete(void* arg)
{
    cmd_tree_node_t* node = (cmd_tree_node_t*)arg;
    free(node->data->name);
    variant_free(node->data->value_range.start);
    variant_free(node->data->value_range.end);
    free(node->data);
    stack_free(node->children);
    free(node);
}

variant_stack_t*    create_cmd_vec_n(const char* cmdline, size_t size)
{
    variant_stack_t* cmd_vec = stack_create();
    int index = 0;
    int cmd_index = 0;
    char ch;
    char cmd_part[1024] = {0};

    if(*cmdline == 0)
    {
        return cmd_vec;
    }

    while(index < size && (ch = cmdline[index++]))
    {
        if(ch == ' ')
        {
            if(cmd_index > 0)
            {
                cmd_part[cmd_index] = 0;
                stack_push_back(cmd_vec, variant_create_string(strdup(cmd_part)));
                cmd_index = 0;
            }
        }
        else if(ch == '!') // The exclamation point is an alias for "end" command
        {
            cmd_part[cmd_index] = 0;
            stack_push_back(cmd_vec, variant_create_string(strdup("end")));
            cmd_index = 0;
        }
        else if(ch != '\n')
        {
            cmd_part[cmd_index++] = ch;
            if(cmd_index >= 1024)
            {
                printf("Command part too long! gonna craaaaaash!\n");
            }
        }
    }

    if(cmdline[index-1] != ' ')
    {
        cmd_part[cmd_index] = 0;
        stack_push_back(cmd_vec, variant_create_string(strdup(cmd_part)));
    }

    return cmd_vec;
}

variant_stack_t*    create_cmd_vec(const char* cmdline)
{
    variant_stack_t* cmd_vec = stack_create();
    int index = 0;
    int cmd_index = 0;
    char ch;
    char cmd_part[1024] = {0};

    if(*cmdline == 0)
    {
        return cmd_vec;
    }

    while(ch = cmdline[index++])
    {
        if(ch == ' ')
        {
            if(cmd_index > 0)
            {
                cmd_part[cmd_index] = 0;
                stack_push_back(cmd_vec, variant_create_string(strdup(cmd_part)));
                cmd_index = 0;
            }
        }
        else if(ch == '!') // The exclamation point is an alias for "end" command
        {
            cmd_part[cmd_index] = 0;
            stack_push_back(cmd_vec, variant_create_string(strdup("end")));
            cmd_index = 0;
        }
        else if(ch != '\n')
        {
            cmd_part[cmd_index++] = ch;
            if(cmd_index >= 1024)
            {
                printf("Command part too long! gonna craaaaaash!\n");
            }
        }
    }

    if(cmdline[index-1] != ' ')
    {
        cmd_part[cmd_index] = 0;
        stack_push_back(cmd_vec, variant_create_string(strdup(cmd_part)));
    }

    return cmd_vec;
}

cmd_tree_node_t*    cmd_find_node(variant_stack_t* root, const char* cmd_part)
{
    for(int i = 0; i < root->count; i++)
    {
        cmd_tree_node_t* child_node = (cmd_tree_node_t*)variant_get_ptr(stack_peek_at(root, i));
        if(strcmp(child_node->data->name, cmd_part) == 0)
        {
            return child_node;
        }
    }

    return NULL;
}




void cmd_process_variadic_argument(cli_command_t* cmd, variant_stack_t* cmd_vec, int arg_index, variant_stack_t* command_tree)
{
    const char* cmd_part = variant_get_string(stack_peek_at(cmd_vec, arg_index));
    char* saveptr;
    char* dup_cmd_part = strdup(cmd_part);
    char* tok = strtok_r(dup_cmd_part, "|", &saveptr);
    variant_stack_t*    command_tree_root = command_tree;
    variant_stack_t*    saved_root = command_tree_root;

    //printf("TOken index %d\n", arg_index);
    while(NULL != tok)
    {
        cmd_tree_node_t* node = cmd_find_node(command_tree_root, tok);
        
        if(NULL != node)
        {
            //printf("Node with token (%s) was found, search for next...\n", tok);
            saved_root = command_tree_root;
            command_tree_root = node->children;
        }
        else
        {
            // Append new node to root
            //printf("append variadic token to root: %s, ptr: %p\n", tok, command_tree_root);
            cmd_tree_node_t* new_node = malloc(sizeof(cmd_tree_node_t));
            new_node->data = cmd_create_node_data(tok, cmd);
            new_node->children = stack_create();
            stack_push_back(command_tree_root, variant_create_ptr(DT_CMD_NODE, (void*)new_node, &cmd_tree_node_delete));
            saved_root = command_tree_root;
            command_tree_root = new_node->children;
        
            // Special case for terminator nodes
            if(cmd_vec->count-1 == arg_index)
            {
                //printf("append final node root\n");
    
                cmd_tree_node_t* term_node = malloc(sizeof(cmd_tree_node_t));
                term_node->data = cmd_create_final_node_data(cmd);
                term_node->children = stack_create();
                stack_push_back(command_tree_root, variant_create_ptr(DT_CMD_NODE, (void*)term_node, &cmd_tree_node_delete));
            }
        }
    
            // Continue adding commands till the end of the command line
            for(int i = arg_index+1; i < cmd_vec->count; i++)
            {
                const char* cmd_part = variant_get_string(stack_peek_at(cmd_vec, i));
    
                //printf("Next token at index %d is %s\n", i, cmd_part);
                if(strchr((char*)cmd_part, '|') != NULL)
                {
                    //printf("Process variadic args: %sm saveptr = %p (%s)\n", cmd_part, saveptr, saveptr);
                    cmd_process_variadic_argument(cmd, cmd_vec, i, command_tree_root);
                    //printf("-- recursion end, saveptr = %p (%s)\n", saveptr, saveptr);
                    break;
                }
                else
                {
                    // Find the node matching command or, find the last node to which we will append child
                    cmd_tree_node_t* node = cmd_find_node(command_tree_root, cmd_part);
            
                    if(NULL == node)
                    {
                        //printf("append regular token to root: %s, ptr: %p\n", cmd_part, command_tree_root);
                        // Append new node to root
                        cmd_tree_node_t* new_node = malloc(sizeof(cmd_tree_node_t));
                        new_node->data = cmd_create_node_data(cmd_part, cmd);
                        new_node->children = stack_create();
                        stack_push_back(command_tree_root, variant_create_ptr(DT_CMD_NODE, (void*)new_node, &cmd_tree_node_delete));
                        //saved_root = command_tree_root;
                        command_tree_root = new_node->children;
            
                        // Special case for terminator nodes
                        if(cmd_vec->count-1 == i)
                        {
                            //printf("Terminate regular node\n");
                            cmd_tree_node_t* term_node = malloc(sizeof(cmd_tree_node_t));
                            term_node->data = cmd_create_final_node_data(cmd);
                            term_node->children = stack_create();
                            stack_push_back(command_tree_root, variant_create_ptr(DT_CMD_NODE, (void*)term_node, &cmd_tree_node_delete));
                        }
                    }
                    else
                    {
                        //printf("Matching node found, search children\n");
                        // Matching node was found - continue search its children
                        //saved_root = command_tree_root;
                        command_tree_root = node->children;
                    }
                }
        }

        tok = strtok_r(NULL, "|", &saveptr);
        if(NULL != tok)
        {
            //printf("Next token: %s\n", tok);
            command_tree_root = saved_root;
            //printf("Next token: %s, append to: %p\n", tok, command_tree_root);
        }
        else
        {
            //printf("No next tokes\n");
        }
    }

    free(dup_cmd_part);
}





void cmd_create_tree(cli_command_t* cmd, variant_stack_t* command_tree)
{
    variant_stack_t*    cmd_vec = create_cmd_vec(cmd->name);
    variant_stack_t*    command_tree_root = command_tree;

    // For each part of the command create a special node and attach it to 
    // a parent or root of the tree
    for(int i = 0; i < cmd_vec->count; i++)
    {
        const char* cmd_part = variant_get_string(stack_peek_at(cmd_vec, i));
        //printf("CMD_PART: %s\n", cmd_part);

                
        // Lets see if cmd_part is actually |-separated list of options:
        if(strchr((char*)cmd_part, '|') != NULL)
        {
            //printf("Process variadic args: %s\n", cmd_part);
            cmd_process_variadic_argument(cmd, cmd_vec, i, command_tree_root);
            break;
        }
        else
        {
            // Find the node matching command or, find the last node to which we will append child
            cmd_tree_node_t* node = cmd_find_node(command_tree_root, cmd_part);
    
            if(NULL == node)
            {
                // Append new node to root
                cmd_tree_node_t* new_node = malloc(sizeof(cmd_tree_node_t));
                new_node->data = cmd_create_node_data(cmd_part, cmd);
                new_node->children = stack_create();
                stack_push_back(command_tree_root, variant_create_ptr(DT_CMD_NODE, (void*)new_node, &cmd_tree_node_delete));
                //saved_root = command_tree_root;
                command_tree_root = new_node->children;
    
                // Special case for terminator nodes
                if(cmd_vec->count-1 == i)
                {
                    cmd_tree_node_t* term_node = malloc(sizeof(cmd_tree_node_t));
                    term_node->data = cmd_create_final_node_data(cmd);
                    term_node->children = stack_create();
                    stack_push_back(command_tree_root, variant_create_ptr(DT_CMD_NODE, (void*)term_node, &cmd_tree_node_delete));
                }
            }
            else
            {
                // Matching node was found - continue search its children
                //saved_root = command_tree_root;
                command_tree_root = node->children;
            }
        }
    }

    stack_free(cmd_vec);
}

void  cli_install_node(cli_node_t** node, cli_node_t* parent, cli_command_t command_list[], char* name, char* prompt)
{
    cli_install_custom_node(cli_node_list, node, parent, command_list, name, prompt);
}

void    cli_install_custom_node(variant_stack_t* custom_cli_node_list, cli_node_t** node, cli_node_t* parent, cli_command_t command_list[], char* name, char* prompt)
{
    *node = (cli_node_t*)calloc(1, sizeof(cli_node_t));
    (*node)->command_tree = stack_create();
    (*node)->name = strdup(name);
    (*node)->prompt = strdup(prompt);
    (*node)->parent = parent;
    (*node)->context = NULL;

    //for(int i = 0; i < sizeof(*command_list)/sizeof(cli_command_t); i++)
    cli_command_t* cmd;
    int i = 0;
    if(NULL != command_list)
    {
        while(cmd = &(command_list[i++]))
        {
            //cli_command_t* cmd = &(command_list[i]);
    
            if(NULL != cmd->name)
            {
                cmd_create_tree(cmd, (*node)->command_tree);
            }
            else
            {
                break;
            }
        }
        
        //if(*node != root_node)
        if(parent != NULL)
        {
            /*cli_command_t*   end_cmd = malloc(sizeof(cli_command_t));
    
            end_cmd->name = strdup("end");
            end_cmd->help = strdup("End current configuration");
            end_cmd->func = &cli_exit_node;*/
    
            cmd_create_tree(&end_cmd, (*node)->command_tree);
        }
    }

    stack_push_back(custom_cli_node_list, variant_create_ptr(DT_CLI_NODE, (*node), variant_delete_none));
}

void  cli_append_to_node(cli_node_t* node, cli_command_t command_list[])
{
    cli_command_t* cmd;
    int i = 0;
    while(cmd = &(command_list[i++]))
    {
        if(NULL != cmd->name)
        {
            cmd_create_tree(cmd, node->command_tree);
        }
        else
        {
            break;
        }
    }
}

// Execute "end" callback on root node
bool    cli_exit_node(vty_t* vty, variant_stack_t* params)
{
    cmd_tree_node_t* tree_node = cmd_find_node(root_node->command_tree,  "end");

    if(NULL != tree_node)
    {
        tree_node->data->command->func(vty, params);
    }
}

void    cli_assemble_line(variant_stack_t* params, int start, char* out_line)
{
    for(int i = start; i < params->count; i++)
    {
        strcat(out_line, variant_get_string(stack_peek_at(params, i)));
        strcat(out_line, " ");
    }

    int last_char = strlen(out_line)-1;
    out_line[last_char] = '\0';
}

/**
recursively traverse the command stack and save all matching 
commands into matches stack. 
*/
void cmd_find_matches_worker(variant_stack_t** root, variant_stack_t* matches, const char* cmd_part_str)
{
    cmd_tree_node_t* last_node = NULL;

    while(matches->count > 0)
    {
        stack_pop_front(matches);
    }

    stack_for_each((*root), command_tree_node_variant)
    {
        cmd_tree_node_t* node = (cmd_tree_node_t*)variant_get_ptr(command_tree_node_variant);
        if(NULL != cmd_part_str)
        {
            if(node->data->type == TYPE_INT)
            {
                char* endptr;
                int base;
                if(strstr(cmd_part_str, "0x") == cmd_part_str)
                {
                    base = 16;
                }
                else
                {
                    base = 10;
                }

                int num = strtol(cmd_part_str, &endptr, base);
                if(*endptr == '\0')
                {
                    if(!node->data->has_range || *cmd_part_str == 0 || (variant_get_int(node->data->value_range.start) <= num && variant_get_int(node->data->value_range.end) >= num))
                    {
                        stack_push_back(matches, command_tree_node_variant);
                        last_node = node;
                    }
                }
            }
            else if((node->data->type == TYPE_TERM && *cmd_part_str == 0) || node->data->type == TYPE_WORD || node->data->type == TYPE_LINE || node->data->type == TYPE_CHAR ||
               (node->data->type == TYPE_CMD_PART && strstr(node->data->name, cmd_part_str) == node->data->name))
            {
                stack_push_back(matches, command_tree_node_variant);

                if(node->data->type != TYPE_LINE)
                {
                    last_node = node;
                }
            }
        }
        else
        {
            stack_push_back(matches, command_tree_node_variant);
        }
    }

    if(NULL != last_node)
    {
        *root = last_node->children;
    }
}

char**  cmd_find_matches(cli_node_t* current_node, variant_stack_t* cmd_vec, variant_stack_t* matches, int* complete_status)
{
    static char** cmd_matches = NULL;

    if(cmd_matches)
    {
        free(cmd_matches);
    }
    
    variant_stack_t* root = current_node->command_tree;

    // Traverse the cmd vec and find the last command part matching cmd_vec
    stack_for_each(cmd_vec, cmd_part_variant)
    {
        const char* cmd_part_str = variant_get_string(cmd_part_variant);

        if(NULL != cmd_part_str || matches->count == 0)
        {
            cmd_find_matches_worker(&root, matches, cmd_part_str);
            if(matches->count == 0)
            {
                break;
            }
        }
    }
    
    // We want to remove singe <CR> match otherwise it will be appended to the command line!
    if(matches->count == 1)
    {
        variant_t* node_variant = stack_peek_at(matches, 0);
        cmd_tree_node_t* node = (cmd_tree_node_t*)variant_get_ptr(node_variant);

        if(node->data->type == TYPE_TERM)
        {
            stack_pop_front(matches);
        }
        else if(node->data->type != TYPE_CMD_PART)
        {
            stack_push_back(matches, variant_create_string(strdup("")));
        }
    }

    cmd_matches = calloc(matches->count+1, sizeof(char*));
    int i = 0;
    while(matches->count > 0)
    {
        variant_t* node_variant = stack_pop_front(matches);
        if(node_variant->type == DT_STRING)
        {
            cmd_matches[i++] = strdup(variant_get_string(node_variant));
        }
        else
        {
            cmd_tree_node_t* node = (cmd_tree_node_t*)variant_get_ptr(node_variant);
            cmd_matches[i++] = strdup(node->data->name);
        }
    }

    return cmd_matches;
}

CmdMatchStatus cli_get_custom_command(cli_node_t* node, const char* cmdline, cmd_tree_node_t** cmd_node, variant_stack_t** complete_cmd_vec)
{
    CmdMatchStatus match_status = CMD_NO_MATCH;

    variant_stack_t* cmd_vec = create_cmd_vec(cmdline);

    //if(cmd_vec->count == 0 || rl_end && isspace((int)rl_line_buffer[rl_end - 1]))
    {
        stack_push_back(cmd_vec, variant_create_string(NULL));
    }

    variant_stack_t*    matches = stack_create();
    int status;

    variant_stack_t* root = node->command_tree;
    
    if(NULL != complete_cmd_vec)
    {
        *complete_cmd_vec = stack_create();
    }

    // Traverse the cmd vec and find the last command part matching cmd_vec
    stack_for_each(cmd_vec, cmd_part_variant)
    {
        const char* cmd_part_str = variant_get_string(cmd_part_variant);

        if(NULL != cmd_part_str && 0 != *cmd_part_str)
        {
            cmd_find_matches_worker(&root, matches, cmd_part_str);

            if(matches->count == 1)
            {
                match_status = CMD_PARTIAL_MATCH;
                variant_t* matched_node_variant = stack_pop_front(matches);
                cmd_tree_node_t* node = variant_get_ptr(matched_node_variant);
                
                if(NULL != complete_cmd_vec)
                {
                    switch(node->data->type)
                    {
                    case TYPE_CMD_PART:
                        stack_push_back(*complete_cmd_vec, variant_create_string(strdup(node->data->name)));
                        break;
                    case TYPE_WORD:
                    case TYPE_LINE:
                    case TYPE_CHAR:
                        stack_push_back(*complete_cmd_vec, variant_create_string(strdup(cmd_part_str)));
                        break;
                    case TYPE_INT:
                        {
                            int base;
                            if(strstr(cmd_part_str, "0x") == cmd_part_str)
                            {
                                base = 16;
                            }
                            else
                            {
                                base = 10;
                            }
    
                            int num = strtol(cmd_part_str, NULL, base);
                            stack_push_back(*complete_cmd_vec, variant_create_int32(DT_INT32, num));
                        }
                        break;
                    }
                }

                // Now lets see if this is the terminal node. Terminal nodes has only one child - <cr>
                stack_for_each(node->children, term_node_variant)
                {
                    cmd_tree_node_t* term_node = variant_get_ptr(term_node_variant);
                    if(term_node->data->type == TYPE_TERM)
                    {
                        match_status = CMD_FULL_MATCH;
                        *cmd_node = term_node;
                    }
                }
            }
            else if(matches->count == 0)
            {
                match_status = CMD_NO_MATCH;
                break;
            }
            else 
            {
                bool exact_match_found = false;

                // Can happen that more than one partial match found and 
                // one exact match found too
                while(matches->count > 0 && !exact_match_found)
                {
                    variant_t* matched_node_variant = stack_pop_front(matches);
                    cmd_tree_node_t* node = variant_get_ptr(matched_node_variant);

                    if(node->data->type == TYPE_CMD_PART && strcmp(node->data->name, cmd_part_str) == 0)
                    {
                        stack_push_back(*complete_cmd_vec, variant_create_string(strdup(node->data->name)));
                        
                        // Now lets see if this is the terminal node. Terminal nodes has only one child - <cr>
                        stack_for_each(node->children, term_node_variant)
                        {
                            cmd_tree_node_t* term_node = variant_get_ptr(term_node_variant);
                            if(term_node->data->type == TYPE_TERM)
                            {
                                match_status = CMD_FULL_MATCH;
                                *cmd_node = term_node;
                                exact_match_found = true;
                            }
                        }
                    }
                }

                while(matches->count > 0)
                {
                    stack_pop_front(matches);
                }

                if(!exact_match_found)
                {
                    match_status = CMD_PARTIAL_MATCH;
                }
                
                break;
            }
        }
    }

    stack_free(matches);
    stack_free(cmd_vec);

    return match_status;
}

CmdMatchStatus cli_get_command(vty_t* vty, const char* cmdline, cmd_tree_node_t** cmd_node, variant_stack_t** complete_cmd_vec)
{
    cli_get_custom_command(vty->current_node, cmdline, cmd_node, complete_cmd_vec);
}

void     cli_command_describe_norl(vty_t* vty)
{
    cmd_tree_node_t* cmd_node;

    CmdMatchStatus match_status = cli_get_command(vty, vty->buffer, &cmd_node, NULL);

    if(match_status == CMD_FULL_MATCH)
    {
        vty_write(vty, "%s%% %s%s", VTY_NEWLINE(vty), cmd_node->data->command->help, VTY_NEWLINE(vty));
        vty_redisplay(vty, vty->buffer);
    }
    else
    {
        variant_stack_t* completions = cli_get_command_completions(vty, vty->buffer, vty->buf_size);
        int word_count = 0;

        if(NULL != completions)
        {
            vty_write(vty, VTY_NEWLINE(vty));
    
            stack_for_each(completions, matching_command)
            {
                vty_write(vty, "%-20s", variant_get_string(matching_command));
                if(word_count++ >= 3)
                {
                    vty_write(vty, VTY_NEWLINE(vty));
                    word_count = 0;
                }
            }
    
            if(word_count > 0)
            {
                vty_write(vty, VTY_NEWLINE(vty));
            }
    
            vty_redisplay(vty, vty->buffer);
            stack_free(completions);
        }
    }
}

char**  cli_command_completer_norl(vty_t* vty, const char* text, int size)
{
    static char** matched;
    static variant_stack_t* matches = NULL;

    variant_stack_t* cmd_vec;

    char* cmd_name;

    stack_free(matches);
    matches = stack_create();

    cmd_vec = create_cmd_vec(text);

    if(cmd_vec->count == 0 || isspace((int)text[size - 1]))
    {
        stack_push_back(cmd_vec, variant_create_string(NULL));
    }
    
    int status;
    matched = cmd_find_matches(vty->current_node, cmd_vec, matches, &status);
    stack_free(cmd_vec);

    if (matched && matched[0])
    {
        return matched;
    }

    return NULL;
}

variant_stack_t*    cli_get_command_completions(vty_t* vty, const char* buffer, size_t size)
{
    char** cmd_list = cli_command_completer_norl(vty, buffer, size);
    variant_stack_t* completions = NULL;

    if(NULL != cmd_list)
    {
        completions = stack_create();

        char** first_cmd = cmd_list;
        while(NULL != *first_cmd)
        {
            stack_push_back(completions, variant_create_string(strdup(*first_cmd)));
            first_cmd++;
        }
    }

    return completions;
}
