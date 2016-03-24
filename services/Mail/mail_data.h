/*
This file holds all the data needed to operate Mail service
*/
#include <hash.h>

typedef struct template_t
{
    char    end_indicator;
    char*   name;
    char*   template;
} template_t;

typedef struct smtp_data_t
{
    char*   server_name;
    int     port;
    bool    is_smtps;
    char*   username;
    char*   password;
} smtp_data_t;

void        mail_data_add_template(const char* template_name, char end_indicator, const char* template);
void        mail_data_del_template(const char* template_name);
template_t* mail_data_get_template(const char* template_name);

void        mail_data_set_smtp_address(const char* smtp_name, int smtp_port, bool is_smtps);
void        mail_data_set_smtp_auth(const char* user, const char* pass);
void        mail_data_clear_smtp_auth();

void        mail_data_add_recipient(const char* recipient);
void        mail_data_del_recipient(const char* recipient);

