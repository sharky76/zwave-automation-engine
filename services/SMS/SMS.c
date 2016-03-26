#include <service.h>
#include "sms_cli.h"
#include "sms_data.h"
#include <hash.h>
#include <stdio.h>

variant_t* sms_send(service_method_t* method, va_list args);
extern hash_table_t*    phone_table;

void service_create(service_t** service, int service_id)
{
    SERVICE_INIT(SMS, "Sent SMS texts");
    SERVICE_ADD_METHOD(Send, sms_send, 1, "Send SMS message");
    (*service)->get_config_callback = sms_cli_get_config;

    sms_data_load();
}

void service_cli_create(cli_node_t* parent_node)
{
    sms_cli_create(parent_node);
}

typedef struct send_sms_visitor_data_t
{
    variant_t* message;
    const char* sms_gw;
} send_sms_visitor_data_t;

void send_sms_visitor(const char* phone, void* arg)
{
    send_sms_visitor_data_t* data = (send_sms_visitor_data_t*)arg;

    char sms_address[128] = {0};
    snprintf(sms_address, 127, "%s%s", phone, data->sms_gw);
    variant_t* sms_addr_var = variant_create_string(sms_address);
    variant_t* ret = service_call_method("Mail", "SendTo", data->message, sms_addr_var);
    free(sms_addr_var);
    variant_free(ret);
}

variant_t* sms_send(service_method_t* method, va_list args)
{
    variant_t* message_variant = va_arg(args, variant_t*);
    //const char* sms_message = variant_get_string(message_variant);

    carrier_data_t* carrier = sms_data_get_carrier();
    const char* sms_gw = sms_data_get_sms_gw(carrier->country_code, carrier->carrier);

    if(NULL != sms_gw)
    {
        send_sms_visitor_data_t data = {
            .message = message_variant,
            .sms_gw = sms_gw
        };

        variant_hash_for_each_value(phone_table, const char*, send_sms_visitor, &data);
        
    }

    return variant_create_bool(true);
}

