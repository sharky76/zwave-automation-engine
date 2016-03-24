#include "mail_data.h"
#include <crc32.h>
#include <stdlib.h>
#include <string.h>

smtp_data_t*  smtp_data;
hash_table_t* template_table;
hash_table_t* recipient_table;

void delete_template(void* arg)
{
    template_t* t = (template_t*)arg;
    free(t->template);
    free(t->name);
    free(t);
}

void        mail_data_add_template(const char* template_name, char end_indicator, const char* template)
{
    if(NULL != template)
    {
        template_t* new_template = malloc(sizeof(template_t));
        new_template->template = strdup(template);
        new_template->name = strdup(template_name);
        new_template->end_indicator = end_indicator;
    
        variant_hash_insert(template_table, crc32(0, template_name, strlen(template_name)), variant_create_ptr(DT_PTR, new_template, delete_template));
    }
}

void        mail_data_del_template(const char* template_name)
{
    variant_hash_remove(template_table, crc32(0, template_name, strlen(template_name)));
}

template_t* mail_data_get_template(const char* template_name)
{
    return variant_get_ptr(variant_hash_get(template_table, crc32(0, template_name, strlen(template_name))));
}

void        mail_data_set_smtp_address(const char* smtp_name, int smtp_port,  bool is_smtps)
{
    if(smtp_data->server_name != NULL)
    {
        free(smtp_data->server_name);
    }

    smtp_data->server_name = strdup(smtp_name);
    smtp_data->port = smtp_port;
    smtp_data->is_smtps = is_smtps;
}

void        mail_data_set_smtp_auth(const char* user, const char* pass)
{
    mail_data_clear_smtp_auth();

    smtp_data->username = strdup(user);
    smtp_data->password = strdup(pass);
}

void        mail_data_clear_smtp_auth()
{
    free(smtp_data->username);
    free(smtp_data->password);

    smtp_data->username = NULL;
    smtp_data->password = NULL;
}

void        mail_data_add_recipient(const char* recipient)
{
    variant_hash_insert(recipient_table, crc32(0, recipient, strlen(recipient)), variant_create_string(strdup(recipient)));
}

void        mail_data_del_recipient(const char* recipient)
{
    variant_hash_remove(recipient_table, crc32(0, recipient, strlen(recipient)));
}

