#include "cli_auth.h"
#include <string.h>
#include <stdlib.h>
#include "user_manager.h"

cli_node_t* auth_user_node;
cli_node_t* auth_pass_node;
extern cli_node_t*         current_node;

bool cmd_auth_username(vty_t* vty, variant_stack_t* params);
bool cmd_auth_password(vty_t* vty, variant_stack_t* params);
bool cmd_auth_add_user(vty_t* vty, variant_stack_t* params);
bool cmd_auth_del_user(vty_t* vty, variant_stack_t* params);
bool cmd_auth_show_users(vty_t* vty, variant_stack_t* params);

void show_user_helper(user_entry_t* entry, void* arg);

cli_command_t   auth_user_list[] = {
    {"WORD",          cmd_auth_username,              "Username"},
    {NULL,                     NULL,                  NULL}
};

cli_command_t   auth_pass_list[] = {
    {"WORD",        cmd_auth_password,              "Password"},
    {NULL, NULL, NULL}
};

cli_command_t   auth_root_list[] = {
    {"user WORD password WORD",  cmd_auth_add_user,  "Add user"},
    {"no user WORD",             cmd_auth_del_user,  "Delete user"},
    {"show user",                   cmd_auth_show_users,  "List users"},
    {NULL, NULL, NULL}
};

void    cli_auth_init(cli_node_t* parent_node)
{
    cli_install_node(&auth_user_node, NULL, auth_user_list, "auth-user", "Username: ");
    cli_install_node(&auth_pass_node, NULL, auth_pass_list, "auth-pass", "Password: ");
    cli_append_to_node(parent_node, auth_root_list);
}

void    cmd_enter_auth_node(vty_t* vty)
{
    stack_for_each(cli_node_list, cli_node_variant)
    {
        cli_node_t* node = (cli_node_t*)variant_get_ptr(cli_node_variant);

        if(strstr(node->name, "auth-user") == node->name)
        {
            char prompt[256] = {0};
            strncpy(prompt, node->prompt, 255);
            vty_set_prompt(vty, prompt);
            vty->current_node = node;
            break;
        }
    }
}

void    cmd_enter_pass_node(vty_t* vty)
{
    stack_for_each(cli_node_list, cli_node_variant)
    {
        cli_node_t* node = (cli_node_t*)variant_get_ptr(cli_node_variant);

        if(strstr(node->name, "auth-pass") == node->name)
        {
            char prompt[256] = {0};
            strncpy(prompt, node->prompt, 255);
            vty_set_prompt(vty, prompt);
            vty_set_echo(vty, false);
            vty->current_node = node;
            break;
        }
    }
}

bool cmd_auth_username(vty_t* vty, variant_stack_t* params)
{
    // Store username....
    auth_pass_node->context = strdup(variant_get_string(stack_peek_at(params, 0)));

    // Now enter password node
    cmd_enter_pass_node(vty);
}

bool cmd_auth_password(vty_t* vty, variant_stack_t* params)
{
    vty_set_echo(vty, true);
    // verify password.
    // username is in node's context
    const char* pass = variant_get_string(stack_peek_at(params, 0));
    if(user_manager_authenticate(auth_pass_node->context, pass))
    {
    
        // Go to root node on success
        vty_set_authenticated(vty, true);
        cmd_enter_root_node(vty);
    }
    else
    {
        vty_error(vty, "Invalid username/password%s", VTY_NEWLINE(vty));
        cmd_enter_auth_node(vty);
    }

    free(auth_pass_node->context);
    auth_pass_node->context = 0;
}

bool cmd_auth_add_user(vty_t* vty, variant_stack_t* params)
{
    if(vty->type == VTY_FILE)
    {
        // File stores already hashed passwords, no need to hash it again
        user_manager_add_hashed_user(variant_get_string(stack_peek_at(params, 1)), variant_get_string(stack_peek_at(params, 3)));
    }
    else if(!user_manager_add_user(variant_get_string(stack_peek_at(params, 1)), variant_get_string(stack_peek_at(params, 3))))
    {
        vty_error(vty, "Unable to add user%s", VTY_NEWLINE(vty));
    }
}

bool cmd_auth_del_user(vty_t* vty, variant_stack_t* params)
{
    user_manager_del_user(variant_get_string(stack_peek_at(params, 2)));
}

bool cmd_auth_show_users(vty_t* vty, variant_stack_t* params)
{
    user_manager_for_each_user(show_user_helper, vty);
}

void show_user_helper(user_entry_t* entry, void* arg)
{
    vty_t* vty = (vty_t*)arg;
    vty_write(vty, "user %s password %d%s", entry->username, entry->password, VTY_NEWLINE(vty));
}
