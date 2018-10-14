#include "cli_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include <readline/readline.h>
//#include <readline/history.h>
#include <stack.h>
#include <cli.h>
#include <signal.h>
#include <sys/wait.h>
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
#include "cli_homebridge.h"
#include "hash.h"
#include "user_manager.h"

DECLARE_LOGGER(CLI)

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
bool    cmd_pager(vty_t* vty, variant_stack_t* params);
bool    cmd_line_filter(vty_t* vty, variant_stack_t* params);
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
    {"show controller queue",        cmd_show_controller_queue,   "Display the contents of controller job queue"},
    {"list command-class",          cmd_list_command_classes,    "List all supported command classes"},
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
    {"more",                 cmd_pager,                 "Pager"},
    {"grep LINE",            cmd_line_filter,           "Line filter"},
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
    cli_homebridge_init(root_node);

    //current_node = root_node;
}

void    cli_load_config()
{
    char config_loc[512] = {0};
    snprintf(config_loc, 511, "%s/startup-config", global_config.config_location);

    vty_io_data_t* file_data = calloc(1, sizeof(vty_io_data_t));
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

int     cli_command_quit(int count, int key)
{
    keep_running = 0;
    siglongjmp(jmpbuf, 1);
}

void child_cmd_stopped_handler(int sig)
{
    pid_t pid = waitpid((pid_t)(-1), 0, 0);
}

bool    cli_command_exec_custom_node(cli_node_t* node, vty_t* vty, char* line)
{
    cmd_tree_node_t* cmd_node;
    bool retVal = false;

    if(line == 0 || *line == 0 || *line == '\n' || *line == '\r')
    {
        return false;
    }
    
    if(isspace(line[strlen(line) - 1]))
    {
        line[strlen(line) - 1] = 0;
    }

    
    char* pipe_char = rindex(line, '|');
    char* pipe_method = NULL;
    if(NULL != pipe_char && *(pipe_char-1) != '|')
    {
        pipe_method = pipe_char+1;
        *pipe_char = 0;
    }

    if(NULL != pipe_method)
    {
        int pipe_fd[2];
        pipe(pipe_fd);
        
        vty_io_data_t* vty_data = calloc(1, sizeof(vty_io_data_t));
        vty_data->desc.io_pair[IN] = fdopen(pipe_fd[IN], "r");
        vty_data->desc.io_pair[OUT] = fdopen(pipe_fd[OUT], "w");;

        vty_t* vty_pipe = vty_io_create(VTY_STD, vty_data);
        vty_pipe->term_width = vty->term_width;
        vty_pipe->term_height = vty->term_height;

        cli_set_vty(vty_pipe);

        pid_t pid = fork();
        if(0 == pid)
        {
            vty_store_vty(vty_pipe, vty);
            cli_command_exec_custom_node(root_node, vty_pipe, pipe_method);
            exit(0);
        }
        else
        {
            // parent
            struct sigaction sa;
            sa.sa_handler = &child_cmd_stopped_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
            sigaction(SIGCHLD, &sa, 0);

            retVal = cli_command_exec_custom_node(node, vty_pipe, line);

            // Insert EOF char
            char eof_ch = (char)EOF;
            vty_pipe->write_cb(vty_pipe, &eof_ch, 1);

            int status = 0;
            // Wait for pipe to process all input
            pid_t pid = waitpid(pid, &status, 0);

            // Recover
            cli_set_vty(vty);
            vty_free(vty_pipe);
            return retVal;
        }
    }

        
    variant_stack_t* params;
    CmdMatchStatus match_status = cli_get_custom_command(node, line, &cmd_node, &params);


    switch(match_status)
    {
    case CMD_FULL_MATCH:
        {
            retVal = cmd_node->data->command->func(vty, params);
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
    return retVal;
}

bool    cli_command_exec(vty_t* vty, char* line)
{
    cli_command_exec_custom_node(vty->current_node, vty, line);
}

void    cli_command_exec_default(char* line)
{
    cli_command_exec(default_vty, line);
}

void    cli_commands_handle_connect_event(int cli_socket, void* context)
{
    int session_sock = accept(cli_socket, NULL, NULL);
                        
    vty_io_data_t* vty_data = calloc(1, sizeof(vty_io_data_t));
    vty_data->desc.socket = session_sock;
    vty_t* vty_sock = vty_io_create(VTY_SOCKET, vty_data); 
    
    //stack_push_back(client_socket_list, variant_create_ptr(DT_PTR, vty_sock, variant_delete_none));
    logger_register_target(vty_sock);
    LOG_ADVANCED(CLI, "Remote client connected");

    cli_set_vty(vty_sock);

    vty_display_banner(vty_sock);
        
    if(user_manager_get_count() > 0)
    {
        cmd_enter_auth_node(vty_sock);
    }

    vty_display_prompt(vty_sock);

    event_register_fd(session_sock, &cli_commands_handle_data_event, (void*)vty_sock);
}

void    cli_commands_handle_data_event(int cli_socket, void* context)
{
    vty_t* vty_ptr = (vty_t*)context;

    char* str = vty_read(vty_ptr);
    
    if(vty_is_command_received(vty_ptr))
    {
        vty_new_line(vty_ptr);
        cli_command_exec(vty_ptr, str);
        vty_display_prompt(vty_ptr);
        vty_flush(vty_ptr);
    }
    else if(vty_is_error(vty_ptr))
    {
        LOG_ERROR(CLI, "Socket error");
        logger_unregister_target(vty_ptr);
        event_unregister_fd(cli_socket);
        vty_free(vty_ptr);
    }

    if(vty_is_shutdown(vty_ptr))
    {
        LOG_ADVANCED(CLI, "Remote client disconnected");
        logger_unregister_target(vty_ptr);
        event_unregister_fd(cli_socket);
        vty_free(vty_ptr);
    }
}

void    cli_commands_handle_http_data_event(int socket, void* context)
{
    vty_t* vty_ptr = (vty_t*)context;
    vty_ptr->current_node = root_node;
    char* str = vty_read(vty_ptr);
    
    if(NULL != str)
    {
        // Skip the first 4 chars "GET "
        cli_command_exec(vty_ptr, str+4);
    }

    event_unregister_fd(socket);
    vty_free(vty_ptr);
}


void    controller_inclusion_wait_handler(variant_t* event_data, void* context)
{
    vty_t* vty = (vty_t*)context;

    if(NULL == event_data)
    {
        vty_write(vty, "%% No new device discovered%s", VTY_NEWLINE(vty));
    }
    else
    {
        vty_write(vty, "%% Discovered new device with ID: %d%s", variant_get_int(event_data), VTY_NEWLINE(vty));
        variant_free(event_data);
    }

    zway_controller_add_node_to_network(zway, FALSE);
}

bool    cmd_controller_inclusion_mode(vty_t* vty, variant_stack_t* params)
{
    const char* mode = variant_get_string(stack_peek_at(params, 2));
    ZWError err;

    if(strcmp(mode, "start") == 0)
    {
        err = zway_controller_add_node_to_network(zway, TRUE);
        vty_write(vty, "%% Inclusion started%s", VTY_NEWLINE(vty));
        USING_LOGGER(DeviceCallback)
        event_wait_async("DeviceAddedEvent", 260, controller_inclusion_wait_handler, (void*)vty);
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

bool    cmd_pager(vty_t* vty, variant_stack_t* params)
{
    int rows = 0;
    char* ch = NULL;
    int n = 0;
    bool is_quit = false;

    do
    {
        do
        {
            if(NULL != ch)
            {
                free(ch);
            }

            n = vty->read_cb(vty, &ch);
            while(ch[0] != 0xa && ch[0] != 0xd && ch[0] != (char)EOF)
            {
                vty_write(vty->stored_vty, "%c", *ch);
                free(ch);
                n = vty->read_cb(vty, &ch);
            }
            
            vty_write(vty->stored_vty, "%s", VTY_NEWLINE(vty->stored_vty));
        }
        while(++rows < vty->term_height && ch[0] != (char)EOF && n > 0);

        if(ch[0] != (char)EOF && n > 0)
        {
            rows = 0;
            vty_write(vty->stored_vty, "-- MORE --");
            
            char* c = 0;
            vty->stored_vty->read_cb(vty->stored_vty, &c);
            while(c[0] != 0xa && c[0] != 0xd && c[0] != ' ' && c[0] != 'q' && c[0] != 'Q')
            {
                free(c);
                vty->stored_vty->read_cb(vty->stored_vty, &c);
            }

            if(c[0] == 'q' || c[0] == 'Q')
            {
                vty_new_line(vty->stored_vty);
                is_quit = true;
            }

            free(c);
        }

    } while(ch[0] != (char)EOF && n > 0 && !is_quit);
    free(ch);
}

bool    cmd_line_filter(vty_t* vty, variant_stack_t* params)
{
    bool isOk;
    char expression[512] = {0};
    cli_assemble_line(params, 1, expression, 511);

    char buf[512] = {0};
    char* ch = NULL;
    int n = 0;

    do
    {
        if(NULL != ch)
        {
            free(ch);
        }

        n = vty->read_cb(vty, &ch);
        while(ch[0] != 0xa && ch[0] != 0xd && ch[0] != (char)EOF)
        {
            strcat(buf, ch);
            free(ch);
            n = vty->read_cb(vty, &ch);
        }
        
        if(strstr(buf, expression) != NULL)
        {
            vty_write(vty->stored_vty, "%s%s", buf, VTY_NEWLINE(vty->stored_vty));
        }

        memset(buf, 0, 512);
     }
     while(ch[0] != (char)EOF && n > 0);
     free(ch);
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
    cli_command_exec(vty, "show sensor descriptor");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show service");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show virtual-device");
    vty_write(vty, "!%s", VTY_NEWLINE(vty));
    cli_command_exec(vty, "show homebridge");
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

    vty_io_data_t* file_vty_data = malloc(sizeof(vty_io_data_t));
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

    vty_io_data_t file_vty_data = {
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
    cli_assemble_line(params, 1, expression, 511);
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

