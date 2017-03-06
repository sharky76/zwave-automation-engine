#include "mail_cli.h"
#include "mail_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vty.h>

char** config_list = NULL;

cli_node_t* mail_node;

//extern void    vty_set_multiline(vty_t* vty, bool is_multiline, char stop_char);
//extern char*   vty_read(vty_t* vty);
//extern void    vty_write(vty_t* vty, const char* format, ...);

extern smtp_data_t*  smtp_data;
extern hash_table_t* template_table;
extern hash_table_t* recipient_table;

bool    cmd_configure_server(vty_t* vty, variant_stack_t* params);
bool    cmd_configure_server_smtps(vty_t* vty, variant_stack_t* params);

bool    cmd_configure_server_auth(vty_t* vty, variant_stack_t* params);
bool    cmd_remove_server_auth(vty_t* vty, variant_stack_t* params);
bool    cmd_add_mail_template(vty_t* vty, variant_stack_t* params);
bool    cmd_del_mail_template(vty_t* vty, variant_stack_t* params);

bool    cmd_add_mail_recipient(vty_t* vty, variant_stack_t* params);
bool    cmd_del_mail_recipient(vty_t* vty, variant_stack_t* params);

cli_command_t    mail_command_list[] = {
    {"server WORD port INT",       cmd_configure_server,   "Configure SMTP server"},
    {"server WORD smtps port INT", cmd_configure_server_smtps,   "Configure SMTP server"},
    {"auth WORD password WORD",     cmd_configure_server_auth,      "Configure SMTP authentication"},
    {"no auth WORD",                cmd_remove_server_auth,      "Configure SMTP authentication"},
    {"template WORD CHAR",          cmd_add_mail_template,      "Add mail template"},
    {"no template WORD",            cmd_del_mail_template,      "Delete mail template"},
    {"recipient WORD",              cmd_add_mail_recipient,     "Add mail receipient"},
    {"no recipient WORD",           cmd_del_mail_recipient,     "Delete mail receipient"},
    {NULL,  NULL,   NULL}
};

void mail_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&mail_node, parent_node, mail_command_list, "Mail", "service-mail");
    template_table = variant_hash_init();
    smtp_data = calloc(1, sizeof(smtp_data_t));
    recipient_table = variant_hash_init();
}


typedef struct show_template_visitor_data_t
{
    char** config_list;
    int    start_index;
    vty_t* vty;
} show_template_visitor_data_t;

void add_to_config_list_visitor(template_t* template, void* arg)
{
    show_template_visitor_data_t* data = (show_template_visitor_data_t*)arg;

    vty_write(data->vty, " template %s %c%s", template->name, template->end_indicator, VTY_NEWLINE(data->vty));
    vty_write_multiline(data->vty, template->template);
    vty_write(data->vty, "%c%s", template->end_indicator, VTY_NEWLINE(data->vty));
}

void add_recipients_to_config_list_visitor(const char* recipient, void* arg)
{
    show_template_visitor_data_t* data = (show_template_visitor_data_t*)arg;
    vty_write(data->vty, " recipient %s%s", recipient, VTY_NEWLINE(data->vty));
}

char** mail_cli_get_config(vty_t* vty)
{
    if(smtp_data->server_name != 0)
    {
        if(smtp_data->is_smtps)
        {
            vty_write(vty, " server %s smtps port %d%s", smtp_data->server_name, smtp_data->port, VTY_NEWLINE(vty));
        }
        else
        {
            vty_write(vty, " server %s port %d%s", smtp_data->server_name, smtp_data->port, VTY_NEWLINE(vty));
        }
    }

    if(smtp_data->username != 0)
    {
        vty_write(vty, " auth %s password %s%s", smtp_data->username, smtp_data->password, VTY_NEWLINE(vty));
    }

    show_template_visitor_data_t data = {
        .vty = vty
    };

    variant_hash_for_each_value(recipient_table, const char*, add_recipients_to_config_list_visitor, &data);
    variant_hash_for_each_value(template_table, template_t*, add_to_config_list_visitor, &data);
    return NULL;
}

bool    cmd_configure_server(vty_t* vty, variant_stack_t* params)
{
    const char* name = variant_get_string(stack_peek_at(params, 1));
    int         port = variant_get_int(stack_peek_at(params, 3));

    mail_data_set_smtp_address(name, port, false);
    return true;
}

bool    cmd_configure_server_smtps(vty_t* vty, variant_stack_t* params)
{
    const char* name = variant_get_string(stack_peek_at(params, 1));
    int         port = variant_get_int(stack_peek_at(params, 4));

    mail_data_set_smtp_address(name, port, true);
    return true;
}


bool    cmd_configure_server_auth(vty_t* vty, variant_stack_t* params)
{
    const char* name = variant_get_string(stack_peek_at(params, 1));
    const char* pass = variant_get_string(stack_peek_at(params, 3));

    mail_data_set_smtp_auth(name, pass);
    return true;
}

bool    cmd_remove_server_auth(vty_t* vty, variant_stack_t* params)
{
    mail_data_clear_smtp_auth();
    return true;
}

bool    cmd_add_mail_template(vty_t* vty, variant_stack_t* params)
{
    const char* template_name = variant_get_string(stack_peek_at(params, 1));
    char template_stop_char = *variant_get_string(stack_peek_at(params, 2));
    vty_write(vty, "Enter template text ending with '%c'%s", template_stop_char, VTY_NEWLINE(vty));

    vty_set_multiline(vty, true, template_stop_char);
    do
    {
        vty_read(vty);
    }
    while(!vty_is_command_received(vty));

    vty_write(vty, VTY_NEWLINE(vty));
    mail_data_add_template(template_name, template_stop_char, vty->buffer);
}

bool    cmd_del_mail_template(vty_t* vty, variant_stack_t* params)
{
    const char* template_name = variant_get_string(stack_peek_at(params, 2));
    mail_data_del_template(template_name);
}

bool    cmd_add_mail_recipient(vty_t* vty, variant_stack_t* params)
{
    mail_data_add_recipient(variant_get_string(stack_peek_at(params, 1)));
}

bool    cmd_del_mail_recipient(vty_t* vty, variant_stack_t* params)
{
    mail_data_del_recipient(variant_get_string(stack_peek_at(params, 2)));
}

