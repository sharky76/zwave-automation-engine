#include "ifttt_cli.h"
#include "vty.h"
#include "ifttt_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"
#include "service_args.h"
#include <json-c/json.h>

char** config_list = NULL;
extern int DT_IFTTT;
cli_node_t* ifttt_node;

cli_node_t* ifttt_action_node;
variant_stack_t* ifttt_actions;

variant_stack_t* ifttt_action_list;

service_method_t* builtin_service_manager_get_method(const char* service_class, const char* method_name);
char* http_response_get_post_data(vty_t* vty);

bool    cmd_configure_key(vty_t* vty, variant_stack_t* params);
bool    cmd_configure_action(vty_t* vty, variant_stack_t* params);
bool    cmd_remove_action(vty_t* vty, variant_stack_t* params);

bool    cmd_ifttt_action_scene(vty_t* vty, variant_stack_t* params);
bool    cmd_ifttt_action_script(vty_t* vty, variant_stack_t* params);
bool    cmd_ifttt_action_command(vty_t* vty, variant_stack_t* params);

void    variant_delete_action(void* arg)
{
    ifttt_action_t* action = (ifttt_action_t*)arg;
    free(action->url);
    free(action->handler_type);
    free(action->handler_name);
    free(action);
}

cli_command_t    ifttt_command_list[] = {
    {"key WORD",       cmd_configure_key,   "Set IFTTT Maker key"},
    {"action WORD scene|command|script LINE",       cmd_configure_action,   "Add IFTTT action"},
    {"no action WORD",       cmd_remove_action,   "Remove IFTTT action"},
    {NULL, NULL, NULL}
};

void   ifttt_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&ifttt_node, parent_node, ifttt_command_list, "IFTTT", "service-IFTTT");

    ifttt_actions = stack_create();
    cli_install_custom_node(ifttt_actions, &ifttt_action_node, NULL, NULL, "", "actions");

    ifttt_action_list = stack_create();
}

char** ifttt_cli_get_config(vty_t* vty)
{
    /*if(NULL != config_list)
    {
        char* cfg;
        int i = 0;
        while(cfg = config_list[i++])
        {
            free(cfg);
        }

        free(config_list);
    }

    int config_index = 0;
    config_list = calloc(2, sizeof(char*));

    if(NULL != ifttt_key)
    {
        char buf[512] = {0};
        snprintf(buf, 511, "key %s", ifttt_key);
        config_list[0] = strdup(buf);
    }

    return config_list;*/

    vty_write(vty, " key %s%s", ifttt_key, VTY_NEWLINE(vty));

    stack_for_each(ifttt_action_list, action_variant)
    {
        ifttt_action_t* action = VARIANT_GET_PTR(ifttt_action_t, action_variant);
        vty_write(vty, " action %s %s %s%s", action->url, action->handler_type, action->handler_name, VTY_NEWLINE(vty));
    }

    return NULL;
}

bool    cmd_configure_key(vty_t* vty, variant_stack_t* params)
{
    const char* key = variant_get_string(stack_peek_at(params, 1));
    
    if(NULL != ifttt_key)
    {
        free(ifttt_key);
    }

    ifttt_key = strdup(key);
    return true;
}

bool    cmd_configure_action(vty_t* vty, variant_stack_t* params)
{
    const char* action_url = variant_get_string(stack_peek_at(params, 1));
    const char* handler_type = variant_get_string(stack_peek_at(params, 2));
    const char* handler_name = variant_get_string(stack_peek_at(params, 3));

    char* action_command = calloc(256, sizeof(char));
    snprintf(action_command, 255, "POST ifttt %s", action_url);     

    cli_command_t* action_command_list = calloc(2, sizeof(cli_command_t));
    action_command_list[0].name = action_command;


    if(strcmp(handler_type, "scene") == 0)
    {
        action_command_list[0].func=cmd_ifttt_action_scene;
    }
    else if(strcmp(handler_type, "script") == 0)
    {
        action_command_list[0].func=cmd_ifttt_action_script;
    }
    else if(strcmp(handler_type, "command") == 0)
    {
        action_command_list[0].func=cmd_ifttt_action_command;
    }

    action_command_list[0].help=strdup("Execute IFTTT action");

    cli_append_to_node(ifttt_action_node, action_command_list);

    ifttt_action_t* action = malloc(sizeof(ifttt_action_t));
    action->url = strdup(action_url);
    action->handler_type = strdup(handler_type);
    action->handler_name = strdup(handler_name);

    stack_push_back(ifttt_action_list, variant_create_ptr(DT_PTR, action, &variant_delete_action));

    return true;
}

bool    cmd_remove_action(vty_t* vty, variant_stack_t* params)
{
    const char* action_url = variant_get_string(stack_peek_at(params, 2));

    stack_for_each(ifttt_action_list, action_variant)
    {
        ifttt_action_t* action = VARIANT_GET_PTR(ifttt_action_t, action_variant);
        if(strcmp(action->url, action_url) == 0)
        {
            stack_remove(ifttt_action_list, action_variant);
            variant_free(action_variant);
            break;
        }
    }
    return true;
}

bool    cmd_ifttt_action_scene(vty_t* vty, variant_stack_t* params)
{
    const char* action_url = variant_get_string(stack_peek_at(params, 2));

    stack_for_each(ifttt_action_list, action_variant)
    {
        ifttt_action_t* action = VARIANT_GET_PTR(ifttt_action_t, action_variant);
        if(strcmp(action->url, action_url) == 0)
        {
            LOG_ADVANCED(DT_IFTTT, "IFTTT scene action matched: %s", action->handler_name);
            const char* json_data = http_response_get_post_data(vty);
            LOG_DEBUG(DT_IFTTT, "Action: %s handler_type: %s, handler_name: %s, post data: %s", action->url, action->handler_type, action->handler_name, json_data);
            
            service_args_stack_destroy(action->handler_name);
            service_args_stack_create(action->handler_name);
            struct json_object* request_obj = json_tokener_parse(json_data);
            
            json_object_object_foreach(request_obj, key, value)
            {
                enum json_type type = json_object_get_type(value);
                variant_t* value_variant = NULL;
                switch(type)
                {
                case json_type_boolean:
                    value_variant = variant_create_bool(json_object_get_boolean(value));
                    break;
                case json_type_double:
                    value_variant = variant_create_float(json_object_get_double(value));
                    break;
                case json_type_int:
                    value_variant = variant_create_int32(DT_INT32, json_object_get_int(value));
                    break;
                case json_type_string:
                    value_variant = variant_create_string(strdup(json_object_get_string(value)));
                    break;
                }

                if(NULL != value_variant)
                {
                    service_args_stack_add(action->handler_name, key, value_variant);
                }
            }
            
            // Now call scene...
            service_post_event(DT_IFTTT, SCENE_ACTIVATION_EVENT, variant_create_string(strdup(action->handler_name)));

            break;
        }
    }

    return true;

}

bool    cmd_ifttt_action_script(vty_t* vty, variant_stack_t* params)
{

}

bool    cmd_ifttt_action_command(vty_t* vty, variant_stack_t* params)
{
    const char* action_url = variant_get_string(stack_peek_at(params, 2));

    stack_for_each(ifttt_action_list, action_variant)
    {
        ifttt_action_t* action = VARIANT_GET_PTR(ifttt_action_t, action_variant);
        if(strcmp(action->url, action_url) == 0)
        {
            LOG_ADVANCED(DT_IFTTT, "IFTTT command action matched: %s", action->handler_name);
            const char* json_data = http_response_get_post_data(vty);
            LOG_DEBUG(DT_IFTTT, "Action: %s handler_type: %s, handler_name: %s, post data: %s", action->url, action->handler_type, action->handler_name, json_data);

            if (NULL != json_data)
            {
                // Prepare command template arguments...
                service_args_stack_destroy("Expression.ProcessTemplate");
                service_args_stack_create("Expression.ProcessTemplate");
                struct json_object *request_obj = json_tokener_parse(json_data);

                if (NULL == request_obj)
                {
                    LOG_ERROR(DT_IFTTT, "Unable to process request data");
                }
                else
                {
                    json_object_object_foreach(request_obj, key, value)
                    {
                        enum json_type type = json_object_get_type(value);
                        variant_t *value_variant = NULL;
                        switch (type)
                        {
                        case json_type_boolean:
                            value_variant = variant_create_bool(json_object_get_boolean(value));
                            break;
                        case json_type_double:
                            value_variant = variant_create_float(json_object_get_double(value));
                            break;
                        case json_type_int:
                            value_variant = variant_create_int32(DT_INT32, json_object_get_int(value));
                            break;
                        case json_type_string:
                            value_variant = variant_create_string(strdup(json_object_get_string(value)));
                            break;
                        }

                        if (NULL != value_variant)
                        {
                            service_args_stack_add("Expression.ProcessTemplate", key, value_variant);
                        }
                    }

                    json_object_put(request_obj);
                }
            }

            // Now process the command template
            service_method_t* process_template_method = builtin_service_manager_get_method("Expression", "ProcessTemplate");
            variant_t* template_var = variant_create_string(strdup(action->handler_name));
            variant_t* processed_expression = service_eval_method(process_template_method, template_var);
            variant_free(template_var);
            LOG_DEBUG(DT_IFTTT, "Processed expression: %s", variant_get_string(processed_expression));

            // Now call command...
            service_post_event(DT_IFTTT, COMMAND_ACTIVATION_EVENT, processed_expression);

            break;
        }
    }

    return true;
}

