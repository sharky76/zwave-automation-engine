#include <service.h>
#include "sms_cli.h"
#include "sms_data.h"

variant_t* sms_send(service_method_t* method, va_list args);

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

variant_t* sms_send(service_method_t* method, va_list args)
{

}

