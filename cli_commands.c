#include "cli_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stack.h>
//#include <cli.h>
#include <ZWayLib.h>
#include <ZData.h>
#include <ZPlatform.h>
#include "variant_types.h"
#include "resolver.h"
#include "command_class.h"
//#include "scene_manager.h"
#include "cli_resolver.h"
#include "cli_scene.h"
#include "cli_sensor.h"
#include "cli_service.h"
#include "cli_logger.h"
#include <setjmp.h>
#include "config.h"

extern sigjmp_buf jmpbuf;
extern int keep_running;

// Forward declaration of commands
bool    cmd_controller_reset(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_inclusion_mode(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_exclusion_mode(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_reset(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_factory_reset(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_remove_failed_node(vty_t* vty, variant_stack_t* params);
bool    cmd_show_controller_queue(vty_t* vty, variant_stack_t* params);
bool    cmd_list_command_classes(vty_t* vty, variant_stack_t* params);


bool    cmd_help(vty_t* vty, variant_stack_t* params);
bool    cmd_set_prompt(vty_t* vty, variant_stack_t* params);
bool    cmd_enter_node(vty_t* vty, variant_stack_t* params);
bool    cmd_exit_node(vty_t* vty, variant_stack_t* params);
bool    cmd_close_session(vty_t* vty, variant_stack_t* params);
bool    cmd_show_running_config(vty_t* vty, variant_stack_t* params);
bool    cmd_save_running_config(vty_t* vty, variant_stack_t* params);
bool    cmd_show_history(vty_t* vty, variant_stack_t* params);
bool    cmd_quit(vty_t* vty, variant_stack_t* params);

void    show_command_class_helper(command_class_t* command_class, void* arg);

cli_command_t root_command_list[] = {
    {"controller inclusion start", cmd_controller_inclusion_mode, "Start inclusion mode"},
    {"controller inclusion stop", cmd_controller_inclusion_mode, "Stop inclusion mode"},
    {"controller exclusion start", cmd_controller_exclusion_mode, "Start exclusion mode"},
    {"controller exclusion stop", cmd_controller_exclusion_mode, "Stop exclusion mode"},
    {"controller reset",          cmd_controller_reset,          "Reset zwave controller"},
    {"controller factory-reset",     cmd_controller_factory_reset,          "Reset zwave controller to factory defaults"},
    {"controller remove-failed node-id INT", cmd_controller_remove_failed_node, "Remove failed node from the controller"},
    {"show controller queue",       cmd_show_controller_queue,   "Display the contents of controller job queue"},
    {"show command-class",          cmd_list_command_classes,    "List all supported command classes"},
    {"help",                 cmd_help,                      "(module) Display help for module"},
    {"prompt LINE",          cmd_set_prompt,                "Set new prompt"},
    {"show running-config",  cmd_show_running_config,       "Show running configuration"},
    {"copy running-config startup-config", cmd_save_running_config, "Save running config into startup config"},
    {"copy running-config file WORD", cmd_show_running_config, "Save running config into custom location"},
    {"show history",                       cmd_show_history,    "Show command history"},
    {"end",                  cmd_exit_node,             "End configuration session"},
    {"exit",                 cmd_quit,                  "Exit the application"},
    {NULL,                   NULL,                          NULL}
};

/*typedef enum
{
    TYPE_CMD_PART,
    TYPE_WORD,
    TYPE_LINE,
    TYPE_INT,
    TYPE_TERM
} NodeType;

typedef enum
{
    CMD_FULL_MATCH,
    CMD_PARTIAL_MATCH,
    CMD_NO_MATCH
} CmdMatchStatus;

typedef struct node_data_t
{
    NodeType    type;
    //bool        is_term;
    char*       name;
    cli_command_t*  command;
} node_data_t;

typedef struct cmd_tree_node_t
{
    node_data_t*        data;
    variant_stack_t*    children;
} cmd_tree_node_t;
*/

cli_node_t*         current_node;
extern ZWay         zway;
//variant_stack_t*    cli_node_list;
struct vty_t*   vty;

void    cli_set_vty(vty_t* new_vty)
{
    vty = new_vty;
    vty_set_prompt(vty, "(%s)# ", root_node->prompt);

}

/*node_data_t*    cmd_create_node_data(const char* cmd_part, cli_command_t* cmd)
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
}*/

void    cli_init()
{
    cli_node_list = stack_create();


    cli_install_node(&root_node, NULL, root_command_list, "", "config");
    

    cli_scene_init(root_node);
    cli_resolver_init(root_node);
    cli_sensor_init(root_node);
    cli_service_init(root_node);
    cli_logger_init(root_node);

    current_node = root_node;
}

void    cli_load_config()
{
    char config_loc[512] = {0};
    snprintf(config_loc, 511, "%s/startup-config", global_config.config_location);

    vty_data_t file_data = {
        .desc.file = fopen(config_loc, "r")
    };

    if(NULL != file_data.desc.file)
    {
        vty_t* file_vty = vty_create(VTY_FILE, &file_data);
        vty_set_prompt(file_vty, "(%s)# ", root_node->prompt);
        char* str;

        while(str = vty_read(file_vty))
        {
            cli_command_exec(file_vty, str);
            free(str);
        }
    }
}

/*
    Completes the provided command
 
    cmd_vec can contain: co inc
*/
/** 
 * 
 * 
 * @param matches
 * @param root
 */

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
                    stack_push_back(matches, command_tree_node_variant);
                    last_node = node;
                }
            }
            else if((node->data->type == TYPE_TERM && *cmd_part_str == 0) || node->data->type == TYPE_WORD || node->data->type == TYPE_LINE ||
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

char**  cmd_find_matches(variant_stack_t* cmd_vec, variant_stack_t* matches, int* complete_status)
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

char*   command_generator(const char* text, int state)
{
    static int completer_cmd_index;
    static int completer_text_len;
    static char** matched;
    static int command_level = 0;
    static variant_stack_t* matches = NULL;

    variant_stack_t* cmd_vec;

    char* cmd_name;

    if(0 == state )
    {
        stack_free(matches);
        matches = stack_create();

        completer_cmd_index = 0;
        completer_text_len = strlen(rl_line_buffer);

        //printf("Buffer: %s\n", rl_line_buffer);

        cmd_vec = create_cmd_vec(rl_line_buffer);

        if(cmd_vec->count == 0 || rl_end && isspace((int)rl_line_buffer[rl_end - 1]))
        {
            stack_push_back(cmd_vec, variant_create_string(NULL));
        }
        
        int status;
        matched = cmd_find_matches(cmd_vec, matches, &status);
        stack_free(cmd_vec);
    }

    if (matched && matched[completer_cmd_index])
    {
        return matched[completer_cmd_index++];
    }

    return NULL;
}

char**   cli_command_completer(const char* text, int start, int stop)
{
    char** matches;

    matches = rl_completion_matches(text, command_generator);

    if (matches)
    {
      rl_point = rl_end;
//      if (complete_status != CMD_COMPLETE_FULL_MATCH)
        /* only append a space on full match */
//        rl_completion_append_character = '\0';
    }

    return matches;
}

CmdMatchStatus cli_get_command(const char* cmdline, cmd_tree_node_t** cmd_node, variant_stack_t** complete_cmd_vec)
{
    CmdMatchStatus match_status = CMD_NO_MATCH;

    variant_stack_t* cmd_vec = create_cmd_vec(cmdline);

    //if(cmd_vec->count == 0 || rl_end && isspace((int)rl_line_buffer[rl_end - 1]))
    {
        stack_push_back(cmd_vec, variant_create_string(NULL));
    }

    variant_stack_t*    matches = stack_create();
    int status;

    variant_stack_t* root = current_node->command_tree;
    
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
                while(matches->count > 0)
                {
                    stack_pop_front(matches);
                }

                match_status = CMD_PARTIAL_MATCH;
                
                break;
            }
        }
    }

    stack_free(matches);
    stack_free(cmd_vec);

    return match_status;
}
/*
Show help for command
*/
int     cli_command_describe()
{
    cmd_tree_node_t* cmd_node;

    CmdMatchStatus match_status = cli_get_command(rl_line_buffer, &cmd_node, NULL);

    if(match_status == CMD_FULL_MATCH)
    {
        vty_write(vty, "\n%% %s\n", cmd_node->data->command->help);
        rl_on_new_line();
    }
    else
    {
        rl_possible_completions(0, '\t');
    }
}

int     cli_command_quit(int count, int key)
{
    keep_running = 0;
    siglongjmp(jmpbuf, 1);
}

bool    cli_command_exec(vty_t* vty, const char* line)
{
    cmd_tree_node_t* cmd_node;

    if(line == 0 || *line == 0)
    {
        return false;
    }
    
    variant_stack_t* params;
    CmdMatchStatus match_status = cli_get_command(line, &cmd_node, &params);

    switch(match_status)
    {
    case CMD_FULL_MATCH:
        {
            //variant_stack_t* params = create_cmd_vec(line);
            cmd_node->data->command->func(vty, params);
            //stack_free(params);
        }
        break;
    case CMD_PARTIAL_MATCH:
        vty_error(vty, "Ambiguous command\n");
        break;
    case CMD_NO_MATCH:
        vty_error(vty, "Unknown command\n");
        break;
    }

    stack_free(params);
    rl_on_new_line();
}

void    cli_command_exec_default(char* line)
{
    cli_command_exec(vty, line);
}

/*void    cli_assemble_line(variant_stack_t* params, int start, char* out_line)
{
    for(int i = start; i < params->count; i++)
    {
        strcat(out_line, variant_get_string(stack_peek_at(params, i)));
        strcat(out_line, " ");
    }

    int last_char = strlen(out_line)-1;
    out_line[last_char] = '\0';
}*/

bool    cmd_controller_inclusion_mode(vty_t* vty, variant_stack_t* params)
{
    const char* mode = variant_get_string(stack_peek_at(params, 2));
    ZWError err;

    if(strcmp(mode, "start") == 0)
    {
        err = zway_controller_add_node_to_network(zway, TRUE);
        //err = zway_fc_add_node_to_network(zway, TRUE, TRUE, NULL, NULL, NULL);
    }
    else
    {
        err = zway_controller_add_node_to_network(zway, FALSE);
        //err = zway_fc_add_node_to_network(zway, FALSE, TRUE, NULL, NULL, NULL);
    }

    return (err == 0);
}

bool    cmd_controller_exclusion_mode(vty_t* vty, variant_stack_t* params)
{
    const char* mode = variant_get_string(stack_peek_at(params, 2));
    ZWError err;

    if(strcmp(mode, "start") == 0)
    {
        err = zway_controller_remove_node_from_network(zway, TRUE);
        //err = zway_fc_remove_node_from_network(zway, TRUE, TRUE, NULL, NULL, NULL);
    }
    else
    {
        err = zway_controller_remove_node_from_network(zway, FALSE);
        //err = zway_fc_remove_node_from_network(zway, FALSE, FALSE, NULL, NULL, NULL);
    }

    return (err == 0);
}

bool    cmd_controller_reset(vty_t* vty, variant_stack_t* params)
{
    zway_fc_serial_api_soft_reset(zway,NULL,NULL,NULL);
}

bool    cmd_controller_factory_reset(vty_t* vty, variant_stack_t* params)
{
    zway_fc_set_default(zway,NULL,NULL,NULL);
}

bool    cmd_controller_remove_failed_node(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 3));
    ZWError err = zway_fc_remove_failed_node(zway, node_id, NULL, NULL, NULL);
}

bool    cmd_show_controller_queue(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "Status: D - done, W - waiting for wakeup of the device, S - waiting for security session\n");
    zway_queue_inspect(zway, vty->data->desc.io_pair[1]);
}

bool    cmd_list_command_classes(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-15s%-15s%s\n", "Command Id", "Name", "Methods(Args)");
    command_class_for_each(show_command_class_helper, vty);
}

bool    cmd_help(vty_t* vty, variant_stack_t* params)
{
}

bool    cmd_set_prompt(vty_t* vty, variant_stack_t* params)
{
    char prompt[256] = {0};
    for(int i = 1; i < params->count; i++)
    {
        strcat(prompt, variant_get_string(stack_peek_at(params, i)));
        strcat(prompt, " ");
    }

    free(root_node->prompt);
    root_node->prompt = strdup(prompt);

    int last_char = strlen(prompt)-1;
    prompt[last_char++] = '#';
    prompt[last_char++] = ' ';
    prompt[last_char++] = 0;

    vty_set_prompt(vty, prompt);
}

bool    cmd_enter_node(vty_t* vty, variant_stack_t* params)
{
    const char* node_name = variant_get_string(stack_peek_at(params, 0));
    stack_for_each(cli_node_list, cli_node_variant)
    {
        
        cli_node_t* node = (cli_node_t*)variant_get_ptr(cli_node_variant);

        if(strstr(node->name, node_name) == node->name)
        {
            char prompt[256] = {0};
            strncpy(prompt, vty->prompt, 255);

            // vty->prompt = (config-scene)#
            // node->prompt = env
            // new prompt = (config-scene-env)#
            char* prompt_end = strstr(prompt, ")#");
            *prompt_end = '\0';
            sprintf(prompt_end, "-%s)# ", node->prompt);
            //sprintf(prompt, "(%s-%s)# ", current_node->prompt, node->prompt);
            vty_set_prompt(vty, prompt);
            current_node = node;
            break;
        }
    }
}

bool    cmd_exit_node(vty_t* vty, variant_stack_t* params)
{
    if(root_node != current_node)
    {
        free(current_node->context);
        current_node->context = 0;
        current_node = current_node->parent;
        char prompt[256] = {0};

        if(current_node == root_node)
        {
            sprintf(prompt, "(%s)# ", current_node->prompt);
        }
        else
        {   
            strncpy(prompt, vty->prompt, 255);

            // vty->prompt = (config-scene-env)#
            // current_node->prompt = scene
            char* current_prompt_start = strstr(prompt, current_node->prompt);

            // move strlen(current prompt) to point to the next "-"
            while(*current_prompt_start != '-')
            {
                current_prompt_start++;
            }

            // Now append ")# " 
            *current_prompt_start = '\0';
            strcat(current_prompt_start, ")# ");
            //sprintf(prompt, "(%s)# ", current_node->prompt);
        }
        vty_set_prompt(vty, prompt);
    }

    return true;
}

bool    cmd_close_session(vty_t* vty, variant_stack_t* params)
{

}

bool    cmd_show_running_config(vty_t* vty, variant_stack_t* params)
{
    cli_command_exec(vty, "show logging");
    vty_write(vty, "!\n");
    cli_command_exec(vty, "show resolver");
    vty_write(vty, "!\n");
    cli_command_exec(vty, "show service");
    vty_write(vty, "!\n");
    cli_command_exec(vty, "show scene");
}

bool    cmd_save_running_config(vty_t* vty, variant_stack_t* params)
{
    char config_loc[512] = {0};
    snprintf(config_loc, 511, "%s/startup-config", global_config.config_location);

    vty_data_t file_vty_data = {
        .desc.file = fopen(config_loc, "w")
    };

    vty_t* file_vty = vty_create(VTY_FILE, &file_vty_data);
    cmd_show_running_config(file_vty, NULL);

    vty_free(file_vty);
}

bool    cmd_show_history(vty_t* vty, variant_stack_t* params)
{
    HIST_ENTRY** history_entry = history_list();

    while(*history_entry)
    {
        vty_write(vty, "%s\n", (*history_entry)->line);
        history_entry++;
    }
}

bool    cmd_quit(vty_t* vty, variant_stack_t* params)
{
    cli_command_quit(0, 0);
}

void    show_command_class_helper(command_class_t* command_class, void* arg)
{
    vty_t* vty = (vty_t*)arg;


    vty_write(vty, "%d (0x%x)%-5s %-15s ", command_class->command_id, command_class->command_id, "", command_class->command_name);

    command_method_t* supported_methods = command_class->supported_method_list;

    while(supported_methods->name)
    {
        vty_write(vty, "%s(%d) [%s] ", supported_methods->name, supported_methods->nargs, supported_methods->help);
        supported_methods++;
    }

    vty_write(vty, "\n");
}

