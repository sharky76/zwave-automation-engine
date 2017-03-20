#include "cli_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stack.h>
#include <cli.h>
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
#include "cli_auth.h"
#include <setjmp.h>
#include "config.h"
#include "command_parser.h"
#include "vty_io.h"
#include "cli_vdev.h"
#include "cli_rest.h"

extern sigjmp_buf jmpbuf;
extern int keep_running;

static char    banner_stop_char;
static char*   banner;
static int    history_size;

// Forward declaration of commands
bool    cmd_controller_reset(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_inclusion_mode(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_exclusion_mode(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_reset(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_factory_reset(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_remove_failed_node(vty_t* vty, variant_stack_t* params);
bool    cmd_show_controller_queue(vty_t* vty, variant_stack_t* params);
bool    cmd_list_command_classes(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_set_learn_mode(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_config_save(vty_t* vty, variant_stack_t* params);
bool    cmd_controller_config_restore(vty_t* vty, variant_stack_t* params);

bool    cmd_help(vty_t* vty, variant_stack_t* params);
bool    cmd_set_banner(vty_t* vty, variant_stack_t* params);
bool    cmd_show_banner(vty_t* vty, variant_stack_t* params);
bool    cmd_clear_banner(vty_t* vty, variant_stack_t* params);
bool    cmd_enter_node_by_name(vty_t* vty, const char* node_name);
bool    cmd_enter_node(vty_t* vty, variant_stack_t* params);
bool    cmd_exit_node(vty_t* vty, variant_stack_t* params);
bool    cmd_close_session(vty_t* vty, variant_stack_t* params);
bool    cmd_show_running_config(vty_t* vty, variant_stack_t* params);
bool    cmd_save_running_config(vty_t* vty, variant_stack_t* params);
bool    cmd_copy_running_config(vty_t* vty, variant_stack_t* params);
bool    cmd_show_history(vty_t* vty, variant_stack_t* params);
bool    cmd_quit(vty_t* vty, variant_stack_t* params);
bool    cmd_eval_expression(vty_t* vty, variant_stack_t* params);
bool    cmd_set_history(vty_t* vty, variant_stack_t* params);

void    show_command_class_helper(command_class_t* command_class, void* arg);

cli_command_t root_command_list[] = {
    {"controller inclusion start", cmd_controller_inclusion_mode, "Start inclusion mode"},
    {"controller inclusion stop", cmd_controller_inclusion_mode, "Stop inclusion mode"},
    {"controller exclusion start", cmd_controller_exclusion_mode, "Start exclusion mode"},
    {"controller exclusion stop", cmd_controller_exclusion_mode, "Stop exclusion mode"},
    {"controller reset",          cmd_controller_reset,          "Reset zwave controller"},
    {"controller factory-reset",     cmd_controller_factory_reset,          "Reset zwave controller to factory defaults"},
    {"controller remove-failed node-id INT", cmd_controller_remove_failed_node, "Remove failed node from the controller"},
    {"controller learn-mode start|stop|start-nwi",            cmd_controller_set_learn_mode, "Start learn mode"},
    {"controller save-config",                                cmd_controller_config_save, "Save controller configuration"},
    {"controller restore-config",                             cmd_controller_config_restore, "Restore controller configuration"},
    //{"controller learn-mode stop",            cmd_controller_set_learn_mode, "Stop learn mode"},
    {"show controller queue",       cmd_show_controller_queue,   "Display the contents of controller job queue"},
    {"show command-class",          cmd_list_command_classes,    "List all supported command classes"},
    {"help",                 cmd_help,                      "(module) Display help for module"},
    {"banner CHAR",          cmd_set_banner,                "Set new banner"},
    {"no banner",            cmd_clear_banner,              "Clear banner"},
    {"show banner",          cmd_show_banner,               "Show banner"},
    {"show running-config",  cmd_show_running_config,       "Show running configuration"},
    {"copy running-config startup-config", cmd_save_running_config, "Save running config into startup config"},
    {"copy running-config file WORD", cmd_copy_running_config, "Save running config into custom location"},
    {"list history",                       cmd_show_history,    "List command history"},
    {"show history",                       cmd_show_history,    "Show command history size"},
    {"history INT",                        cmd_set_history, "Set command history size"},
    {"eval LINE",            cmd_eval_expression,       "Evaluate expression"},
    {"end",                  cmd_exit_node,             "End configuration session"},
    {"exit",                 cmd_quit,                  "Exit the application"},
    {NULL,                   NULL,                          NULL}
};

//cli_node_t*         current_node;
extern ZWay         zway;
vty_t*              default_vty;

void    cli_set_vty(vty_t* new_vty)
{
    //vty_t* vty = new_vty;
    vty_set_prompt(new_vty, "(%s)# ", root_node->prompt);

    if(banner != NULL)
    {
        vty_set_banner(new_vty, banner);
    }

    vty_set_history_size(new_vty, history_size);

    new_vty->current_node = root_node;
    default_vty = new_vty;
}

void    cli_init()
{
    cli_node_list = stack_create();


    cli_install_node(&root_node, NULL, root_command_list, "", "config");
    

    cli_scene_init(root_node);
    cli_resolver_init(root_node);
    cli_sensor_init(root_node);
    cli_vdev_init(root_node);
    cli_service_init(root_node);
    cli_logger_init(root_node);
    cli_auth_init(root_node);
    cli_rest_init(root_node);
    //current_node = root_node;
}

void    cli_load_config()
{
    char config_loc[512] = {0};
    snprintf(config_loc, 511, "%s/startup-config", global_config.config_location);

    vty_data_t* file_data = calloc(1, sizeof(vty_data_t));
    file_data->desc.file = fopen(config_loc, "r");

    if(NULL != file_data->desc.file)
    {
        vty_t* file_vty = vty_io_create(VTY_FILE, file_data);
        vty_set_prompt(file_vty, "(%s)# ", root_node->prompt);
        char* str;
        
        cli_set_vty(file_vty);

        while((str = vty_read(file_vty)) && !vty_is_error(file_vty))
        {
            if(vty_is_command_received(file_vty))
            {
                cli_command_exec(file_vty, str);
            }
        }

        vty_free(file_vty);
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
        char** first_cmd = cmd_matches;
        while(NULL != *first_cmd)
        {
            free(*first_cmd);
            first_cmd++;
        }

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
        matched = cmd_find_matches(default_vty->current_node, cmd_vec, matches, &status);
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

char**  cli_command_completer_norl(vty_t* vty, const char* text, int size)
{
    static char** matched;
    static variant_stack_t* matches = NULL;

    variant_stack_t* cmd_vec;

    char* cmd_name;

    stack_free(matches);
    matches = stack_create();

    //printf("Buffer: %s\n", rl_line_buffer);

    cmd_vec = create_cmd_vec(text);

    if(cmd_vec->count == 0 || /*rl_end &&*/ isspace((int)text[size - 1]))
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

/*CmdMatchStatus cli_get_custom_command(cli_node_t* node, const char* cmdline, cmd_tree_node_t** cmd_node, variant_stack_t** complete_cmd_vec)
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
}*/

CmdMatchStatus cli_get_command(vty_t* vty, const char* cmdline, cmd_tree_node_t** cmd_node, variant_stack_t** complete_cmd_vec)
{
    cli_get_custom_command(vty->current_node, cmdline, cmd_node, complete_cmd_vec);
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

/*
Show help for command
*/
int     cli_command_describe()
{
    cmd_tree_node_t* cmd_node;

    CmdMatchStatus match_status = cli_get_command(default_vty, rl_line_buffer, &cmd_node, NULL);

    if(match_status == CMD_FULL_MATCH)
    {
        rl_save_prompt();
        rl_crlf();
        rl_message("%% %s", cmd_node->data->command->help);
        rl_crlf();
        rl_restore_prompt();
        rl_forced_update_display();
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

bool    cli_command_exec_custom_node(cli_node_t* node, vty_t* vty, char* line)
{
    cmd_tree_node_t* cmd_node;

    if(line == 0 || *line == 0 || *line == '\n' || *line == '\r')
    {
        return false;
    }
    
    if(isspace(line[strlen(line) - 1]))
    {
        line[strlen(line) - 1] = 0;
    }

    variant_stack_t* params;
    CmdMatchStatus match_status = cli_get_custom_command(node, line, &cmd_node, &params);

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
        vty_error(vty, "Ambiguous command%s", VTY_NEWLINE(vty));
        break;
    case CMD_NO_MATCH:
        vty_error(vty, "Unknown command%s", VTY_NEWLINE(vty));
        break;
    }

    stack_free(params);
}

bool    cli_command_exec(vty_t* vty, char* line)
{
    cli_command_exec_custom_node(vty->current_node, vty, line);
}

void    cli_command_exec_default(char* line)
{
    cli_command_exec(default_vty, line);
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
        vty_write(vty, "%% Inclusion started%s", VTY_NEWLINE(vty));
        //err = zway_fc_add_node_to_network(zway, TRUE, TRUE, NULL, NULL, NULL);
    }
    else
    {
        err = zway_controller_add_node_to_network(zway, FALSE);
        vty_write(vty, "%% Inclusion stopped%s", VTY_NEWLINE(vty));
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
        vty_write(vty, "%% Exclusion started%s", VTY_NEWLINE(vty));
        //err = zway_fc_remove_node_from_network(zway, TRUE, TRUE, NULL, NULL, NULL);
    }
    else
    {
        err = zway_controller_remove_node_from_network(zway, FALSE);
        vty_write(vty, "%% Exclusion stopped%s", VTY_NEWLINE(vty));

        //err = zway_fc_remove_node_from_network(zway, FALSE, FALSE, NULL, NULL, NULL);
    }

    return (err == 0);
}

bool    cmd_controller_set_learn_mode(vty_t* vty, variant_stack_t* params)
{
    const char* mode = variant_get_string(stack_peek_at(params, 2));
    ZWError err;

    if(strcmp(mode, "start") == 0)
    {
        err = zway_controller_set_learn_mode(zway, 1);
    }
    else if(strcmp(mode, "start-nwi") == 0)
    {
        err = zway_controller_set_learn_mode(zway, 2);
    }
    else if(strcmp(mode, "stop") == 0)
    {
        err = zway_controller_set_learn_mode(zway, 0);
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

bool    cmd_controller_config_save(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE* save_buffer;
    size_t  buffer_len;

    ZWError err = zway_controller_config_save(zway, &save_buffer, &buffer_len);

    if(err == 0)
    {
        char config_loc[512] = {0};
        snprintf(config_loc, 511, "%s/controller-backup.tgz", global_config.config_location);
        FILE* f = fopen(config_loc, "w");
        if(NULL != f)
        {
            if(1 == fwrite(save_buffer, buffer_len, 1, f))
            {
                fclose(f);
                return true;
            }
        }
    }

    return false;
}

bool    cmd_controller_config_restore(vty_t* vty, variant_stack_t* params)
{

}

bool    cmd_controller_remove_failed_node(vty_t* vty, variant_stack_t* params)
{
    ZWBYTE node_id = variant_get_int(stack_peek_at(params, 3));
    ZWError err = zway_fc_remove_failed_node(zway, node_id, NULL, NULL, NULL);
}

bool    cmd_show_controller_queue(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "Status: D - done, W - waiting for wakeup of the device, S - waiting for security session%s", VTY_NEWLINE(vty));

    char queuebuf[65000] = {0};
    FILE* queue_out = fmemopen(queuebuf, 64999, "w+");
    zway_queue_inspect(zway, queue_out);
    fflush(queue_out);
    vty_write_multiline(vty, queuebuf);
}

bool    cmd_list_command_classes(vty_t* vty, variant_stack_t* params)
{
    vty_write(vty, "%-15s%-15s%s%s", "Command Id", "Name", "Methods(Args)", VTY_NEWLINE(vty));
    command_class_for_each(show_command_class_helper, vty);
}

bool    cmd_help(vty_t* vty, variant_stack_t* params)
{
}

bool    cmd_set_banner(vty_t* vty, variant_stack_t* params)
{
    banner_stop_char = *variant_get_string(stack_peek_at(params, 1));
    vty_write(vty, "Enter banner text ending with '%c'%s", banner_stop_char, VTY_NEWLINE(vty));

    free(banner);
    banner = NULL;

    vty_set_multiline(vty, true, banner_stop_char);
    do
    {
        vty_read(vty);
    }
    while(!vty_is_command_received(vty));

    if(NULL != vty->buffer)
    {
        banner = strdup(vty->buffer);
    }

    vty_write(vty, VTY_NEWLINE(vty));
}

bool    cmd_show_banner(vty_t* vty, variant_stack_t* params)
{
    if(0 != banner_stop_char && NULL != banner)
    {
        vty_write(vty, "banner %c%s", banner_stop_char, VTY_NEWLINE(vty));

        vty_write_multiline(vty, banner);

        vty_write(vty, "%c%s", banner_stop_char, VTY_NEWLINE(vty));

        //vty_write(vty, "%s%c%s", banner, banner_stop_char, VTY_NEWLINE(vty));
    }
    else
    {
        vty_write(vty, "no banner%s", VTY_NEWLINE(vty));
    }
}

bool    cmd_clear_banner(vty_t* vty, variant_stack_t* params)
{
    banner_stop_char = 0;
    vty->banner = NULL;
    free(banner);
    banner = NULL;
}

void    cmd_enter_root_node(vty_t* vty)
{
    stack_for_each(cli_node_list, cli_node_variant)
    {
        cli_node_t* node = (cli_node_t*)variant_get_ptr(cli_node_variant);

        if(strstr(node->name, "") == node->name)
        {
            //current_node = node;
            vty->current_node = node;
            char prompt[256] = {0};
            sprintf(prompt, "(%s)# ", vty->current_node->prompt);
            vty_set_prompt(vty, prompt);
            break;
        }
    }
}

bool    cmd_enter_node(vty_t* vty, variant_stack_t* params)
{
    const char* node_name = variant_get_string(stack_peek_at(params, 0));
    cmd_enter_node_by_name(vty, node_name);
}

bool    cmd_enter_node_by_name(vty_t* vty, const char* node_name)
{
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
            //current_node = node;
            vty->current_node = node;
            sprintf(prompt, "(%s)# ", vty->current_node->prompt);
            break;
        }
    }
}

bool    cmd_exit_node(vty_t* vty, variant_stack_t* params)
{
    if(root_node != vty->current_node)
    {
        //free(current_node->context);
        //current_node->context = 0;

        free(vty->current_node->context);
        vty->current_node->context = 0;
        vty->current_node = vty->current_node->parent;
        //current_node = current_node->parent;
        char prompt[256] = {0};

        if(vty->current_node == root_node)
        {
            sprintf(prompt, "(%s)# ", vty->current_node->prompt);
        }
        else
        {   
            strncpy(prompt, vty->prompt, 255);

            // vty->prompt = (config-scene-env)#
            // current_node->prompt = scene
            char* current_prompt_start = strstr(prompt, vty->current_node->prompt);

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
    cli_command_exec(vty, "show banner");
    vty_write(vty, "!%s",VTY_NEWLINE(vty));
    cli_command_exec(vty, "show user");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show history");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show logging");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show resolver");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show service");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show virtual-device");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show scene");
}

bool    cmd_save_running_config(vty_t* vty, variant_stack_t* params)
{
    char config_loc[512] = {0};
    snprintf(config_loc, 511, "%s/startup-config", global_config.config_location);

    char backup_loc[512] = {0};
    snprintf(backup_loc, 511, "%s/startup-config.backup", global_config.config_location);

    // First rename current startup config to backup
    rename(config_loc, backup_loc);

    vty_data_t* file_vty_data = malloc(sizeof(vty_data_t));
    file_vty_data->desc.file = fopen(config_loc, "w");

    vty_t* file_vty = vty_io_create(VTY_FILE, file_vty_data);
    vty_set_banner(file_vty, vty->banner);
    vty_set_history_size(file_vty, vty->history_size);
    file_vty->current_node = vty->current_node;
    cmd_show_running_config(file_vty, NULL);

    vty_free(file_vty);
}

bool    cmd_copy_running_config(vty_t* vty, variant_stack_t* params)
{
    const char* dest_filename = variant_get_string(stack_peek_at(params, 3));

    char config_loc[512] = {0};
    snprintf(config_loc, 511, "%s/%s", global_config.config_location, dest_filename);

    vty_data_t file_vty_data = {
        .desc.file = fopen(config_loc, "w")
    };

    vty_t* file_vty = vty_create(VTY_FILE, &file_vty_data);
    cmd_show_running_config(file_vty, NULL);

    vty_free(file_vty);
}

bool    cmd_show_history(vty_t* vty, variant_stack_t* params)
{
    const char* mode = variant_get_string(stack_peek_at(params, 0));

    if(strcmp(mode, "list") == 0)
    {
        vty_show_history(vty);
    }
    else if(strcmp(mode, "show") == 0)
    {
        vty_write(vty, "history %d%s", vty->history_size, VTY_NEWLINE(vty));
    }
}

bool    cmd_set_history(vty_t* vty, variant_stack_t* params)
{
    history_size = variant_get_int(stack_peek_at(params, 1));
    
    vty_set_history_size(vty, history_size);
}

bool    cmd_quit(vty_t* vty, variant_stack_t* params)
{
    //cli_command_quit(0, 0);
    vty_shutdown(vty);
}

bool    cmd_eval_expression(vty_t* vty, variant_stack_t* params)
{
    bool isOk;
    char expression[512] = {0};
    cli_assemble_line(params, 1, expression);
    variant_stack_t* compiled_value = command_parser_compile_expression(expression, &isOk);
    bool retVal = false;

    if(isOk)
    {
        variant_t* result = command_parser_execute_expression(compiled_value);
        if(NULL != result)
        {
            char* str_result;
            if(variant_to_string(result, &str_result))
            {
                vty_write(vty, "%% %s%s", str_result, VTY_NEWLINE(vty));
                free(str_result);
                variant_free(result);
                retVal = true;
            }
            else
            {
                variant_free(result);
            }
        }
    }

    stack_free(compiled_value);

    if(!retVal)
    {
        vty_error(vty, "Error evaluating expression%s", VTY_NEWLINE(vty));
    }

    return retVal;
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

    vty_write(vty, VTY_NEWLINE(vty));
}

