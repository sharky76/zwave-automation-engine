
typedef struct carrier_data_t
{
    char* country_code;
    char* carrier;
} carrier_data_t;

void    sms_data_load();
const char**  sms_data_get_country_code_list();
const char**  sms_data_get_carrier_list(const char* country_code);
const char*   sms_data_get_sms_gw(const char* country_code, const char* carrier);

void    sms_data_add_phone(const char* number);
void    sms_data_del_phone(const char* number);

void    sms_data_set_carrier(char* country_code, char* carrier);
carrier_data_t* sms_data_get_carrier();

