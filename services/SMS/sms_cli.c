#include "sms_cli.h"
#include "sms_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <hash.h>

char** config_list = NULL;

cli_node_t* sms_node;
extern carrier_data_t*  carrier_data;
extern hash_table_t*    phone_table;

bool    cmd_add_phone_number(vty_t* vty, variant_stack_t* params);
bool    cmd_del_phone_number(vty_t* vty, variant_stack_t* params);
bool    cmd_set_sms_carrier(vty_t* vty, variant_stack_t* params);

cli_command_t    sms_command_list[] = {
    {"phone-number WORD",       cmd_add_phone_number,   "Add phone number"},
    {"no phone-number WORD",       cmd_del_phone_number,   "Delete phone number"},
    //{"country-code WORD carrier WORD", cmd_set_sms_carrier, "Setup SMS carrier"},
    {NULL,  NULL,   NULL}
};

typedef struct phone_table_visitor_t
{
    char** config_list;
    int    start_index;
} phone_table_visitor_t;

void add_to_config_list_visitor(const char* phone, void* arg)
{
    phone_table_visitor_t* data = (phone_table_visitor_t*)arg;

    char buf[128] = {0};
    snprintf(buf, 1023, "phone-number %s", phone);
    data->config_list[data->start_index] = strdup(buf);
    data->start_index++;
}

char** sms_cli_get_config(vty_t* vty)
{
    if(NULL != config_list)
    {
        char* cfg;
        int i = 0;
        while(cfg = config_list[i++])
        {
            free(cfg);
        }

        free(config_list);
    }

    config_list = calloc(phone_table->count + 2, sizeof(char*));

    char buf[128] = {0};
    carrier_data_t* carrier_data = sms_data_get_carrier();

    if(carrier_data->carrier != NULL)
    {
        snprintf(buf, 127, "country-code %s carrier %s", carrier_data->country_code, carrier_data->carrier);
        config_list[0] = strdup(buf);
    
        phone_table_visitor_t data = {
            .config_list = config_list,
            .start_index = 1
        };
    
        variant_hash_for_each_value(phone_table, const char*, add_to_config_list_visitor, &data);
    }

    return config_list;
}

void sms_cli_create(cli_node_t* parent_node)
{
    cli_install_node(&sms_node, parent_node, sms_command_list, "SMS", "service-sms");

    carrier_data = calloc(1, sizeof(carrier_data_t));
    phone_table = variant_hash_init();
    const char** country_code_array = sms_data_get_country_code_list();
    const char** root = country_code_array;

    while(NULL != *country_code_array)
    {
        //printf("Country code: %s\n", *country_code_array);
        const char**  carrier_array = sms_data_get_carrier_list(*country_code_array);
        const char** carrier_root = carrier_array;
        while(NULL != *carrier_array)
        {
            char* full_command = calloc(128, sizeof(char));
            snprintf(full_command, 127, "country-code %s carrier %s", *country_code_array, *carrier_array); 
            carrier_array++;    
    
            cli_command_t* command_list = calloc(2, sizeof(cli_command_t));
            command_list[0].name = full_command;
            command_list[0].func=cmd_set_sms_carrier;
            command_list[0].help=strdup("Setup SMS carrier");
    
            cli_append_to_node(sms_node, command_list);
        }
        free(carrier_root);

        country_code_array++;
    }

    free(root);
}

bool    cmd_add_phone_number(vty_t* vty, variant_stack_t* params)
{
    const char* phone = variant_get_string(stack_peek_at(params, 1));
    sms_data_add_phone(strdup(phone));
}

bool    cmd_del_phone_number(vty_t* vty, variant_stack_t* params)
{
    const char* phone = variant_get_string(stack_peek_at(params, 2));
    sms_data_del_phone(phone);
}

bool    cmd_set_sms_carrier(vty_t* vty, variant_stack_t* params)
{
    const char* country_code = variant_get_string(stack_peek_at(params, 1));
    const char* carrier = variant_get_string(stack_peek_at(params, 3));
    sms_data_set_carrier(strdup(country_code), strdup(carrier));
}

