#include "cli.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    node_data_t* data = malloc(sizeof(node_data_t));

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
    else if(strcmp(cmd_part, "INT") == 0)
    {
        data->type = TYPE_INT;
    }
    else
    {
        data->type = TYPE_CMD_PART;
    }

    return data;
}

node_data_t*    cmd_create_final_node_data(cli_command_t* cmd)
{
    node_data_t* data = malloc(sizeof(node_data_t));
    
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
    free(node->data);
    stack_free(node->children);
    free(node);
}

variant_stack_t*    create_cmd_vec(const char* cmdline)
{
    variant_stack_t* cmd_vec = stack_create();
    int index = 0;
    int cmd_index = 0;
    char ch;
    char cmd_part[128] = {0};

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

void cmd_create_tree(cli_command_t* cmd, variant_stack_t* command_tree)
{
    variant_stack_t*    cmd_vec = create_cmd_vec(cmd->name);
    variant_stack_t*    command_tree_root = command_tree;

    // For each part of the command create a special node and attach it to 
    // a parent or root of the tree
    for(int i = 0; i < cmd_vec->count; i++)
    {
        const char* cmd_part = variant_get_string(stack_peek_at(cmd_vec, i));
        
        // Find the node matching command or, find the last node to which we will append child
        cmd_tree_node_t* node = cmd_find_node(command_tree_root, cmd_part);

        if(NULL == node)
        {
            // Append new node to root
            cmd_tree_node_t* new_node = malloc(sizeof(cmd_tree_node_t));
            new_node->data = cmd_create_node_data(cmd_part, cmd);
            new_node->children = stack_create();
            stack_push_back(command_tree_root, variant_create_ptr(DT_CMD_NODE, (void*)new_node, &cmd_tree_node_delete));
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
            command_tree_root = node->children;
        }
    }

    stack_free(cmd_vec);
}

void  cli_install_node(cli_node_t** node, cli_node_t* parent, cli_command_t command_list[], char* name, char* prompt)
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
        
        if(*node != root_node)
        {
            /*cli_command_t*   end_cmd = malloc(sizeof(cli_command_t));
    
            end_cmd->name = strdup("end");
            end_cmd->help = strdup("End current configuration");
            end_cmd->func = &cli_exit_node;*/
    
            cmd_create_tree(&end_cmd, (*node)->command_tree);
        }
    }

    stack_push_back(cli_node_list, variant_create_ptr(DT_CLI_NODE, (*node), variant_delete_none));
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
