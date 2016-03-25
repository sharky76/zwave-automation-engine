#include <service.h>
#include <stdio.h>
#include "mail_cli.h"
#include "mail_data.h"
#include <curl/curl.h>
#include <logger.h>

extern smtp_data_t*  smtp_data;

variant_t*  mail_send_mail(service_method_t* method, va_list args);
variant_t*  mail_send_mail_to(service_method_t* method, va_list args);
variant_t*  mail_get_template(service_method_t* method, va_list args);

int DT_MAIL;
extern hash_table_t* recipient_table;

void    service_create(service_t** service, int service_id)
{
    SERVICE_INIT(Mail, "Provide SMTP mail sending services");
    SERVICE_ADD_METHOD(Send, mail_send_mail, 1, "Send mail message. Arg: message");
    SERVICE_ADD_METHOD(SendTo, mail_send_mail_to, 2, "Send mail message. Args: message, address");
    SERVICE_ADD_METHOD(GetTemplate, mail_get_template, 1, "Return mail template by name");
    (*service)->get_config_callback = mail_cli_get_config;
    DT_MAIL = service_id;
}

void    service_cli_create(cli_node_t* parent_node)
{
    mail_cli_create(parent_node);
}



typedef struct upload_context_t {
  int bytes_read;
  const char* message;
} upload_context_t;
 
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_context_t *upload_ctx = (upload_context_t *)userp;
  const char *data;
 
  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }
 
  data = upload_ctx->message + upload_ctx->bytes_read;
 
  if(data) {
    size_t len = strlen(data);

    if(len < size*nmemb)
    {
        memcpy(ptr, data, len);
        upload_ctx->bytes_read = len;
    }
    else
    {
        memcpy(ptr, data, size*nmemb);
        upload_ctx->bytes_read += size*nmemb;
        len = size*nmemb;
    }
 
    return len;
  }
 
  return 0;
}

void create_recipient_list(const char* recipient, void* arg)
{
    struct curl_slist **recipients = (struct curl_slist**)arg;
    *recipients = curl_slist_append(*recipients, recipient);
}

variant_t*  mail_send_mail(service_method_t* method, va_list args)
{
    variant_t* message_variant = va_arg(args, variant_t*);

    
    //const char* recepient = variant_get_string(recepient_variant);
    const char* mail_message = variant_get_string(message_variant);

    char mail_payload[128 + strlen(mail_message)];
    sprintf(mail_payload, "From: Home Security System <zae@local>\r\n%s", mail_message);

    CURL *curl_handle;
    CURLcode res;
    struct curl_slist *recipients = NULL;

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */ 
    curl_handle = curl_easy_init();
    char urlbuf[512] = {0};

    /* specify URL to get */ 
    if(smtp_data->is_smtps)
    {
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
     
        /* If the site you're connecting to uses a different host name that what
         * they have mentioned in their server certificate's commonName (or
         * subjectAltName) fields, libcurl will refuse to connect. You can skip
         * this check, but this will make the connection less secure. */ 
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

        /* Set username and password */ 
        curl_easy_setopt(curl_handle, CURLOPT_USERNAME, smtp_data->username);
        curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, smtp_data->password);

        snprintf(urlbuf, 511, "smtps://%s:%d", smtp_data->server_name, smtp_data->port);
    }
    else
    {
        snprintf(urlbuf, 511, "smtp://%s:%d", smtp_data->server_name, smtp_data->port);
    }
    
    LOG_DEBUG(DT_MAIL, "SMTP URL is %s", urlbuf);
    curl_easy_setopt(curl_handle, CURLOPT_URL, urlbuf);
    curl_easy_setopt(curl_handle, CURLOPT_MAIL_FROM, "Home Security System <zae@local>");

    variant_hash_for_each_value(recipient_table, const char*, create_recipient_list, (void*)&recipients);

    curl_easy_setopt(curl_handle, CURLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */ 
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, payload_source);

    upload_context_t context = {
        .bytes_read = 0,
        .message = mail_payload
    };
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, &context);
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);

    /* get it! */ 
    res = curl_easy_perform(curl_handle);

    /* check for errors */ 
    if(res != CURLE_OK) 
    {
        LOG_ERROR(DT_MAIL, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    }
    else 
    {
        LOG_DEBUG(DT_MAIL, "Mail sent successfully");
    }

    /* Free the list of recipients */ 
    curl_slist_free_all(recipients);

    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl_handle);

    return variant_create_bool(true);
}

variant_t*  mail_send_mail_to(service_method_t* method, va_list args)
{
    variant_t* message_variant = va_arg(args, variant_t*);
    variant_t* recipient_variant = va_arg(args, variant_t*);
    
    const char* mail_message = variant_get_string(message_variant);
    const char* recipient = variant_get_string(recipient_variant);

    char mail_payload[128 + strlen(mail_message)];
    sprintf(mail_payload, "From: Home Security System <zae@local>\r\n%s", mail_message);

    CURL *curl_handle;
    CURLcode res;
    struct curl_slist *recipients = NULL;

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */ 
    curl_handle = curl_easy_init();
    char urlbuf[512] = {0};

    /* specify URL to get */ 
    if(smtp_data->is_smtps)
    {
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
     
        /* If the site you're connecting to uses a different host name that what
         * they have mentioned in their server certificate's commonName (or
         * subjectAltName) fields, libcurl will refuse to connect. You can skip
         * this check, but this will make the connection less secure. */ 
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

        /* Set username and password */ 
        curl_easy_setopt(curl_handle, CURLOPT_USERNAME, smtp_data->username);
        curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, smtp_data->password);

        snprintf(urlbuf, 511, "smtps://%s:%d", smtp_data->server_name, smtp_data->port);
    }
    else
    {
        snprintf(urlbuf, 511, "smtp://%s:%d", smtp_data->server_name, smtp_data->port);
    }
    
    LOG_DEBUG(DT_MAIL, "SMTP URL is %s", urlbuf);
    curl_easy_setopt(curl_handle, CURLOPT_URL, urlbuf);
    curl_easy_setopt(curl_handle, CURLOPT_MAIL_FROM, "Home Security System <zae@local>");

    recipients = curl_slist_append(recipients, recipient);
    curl_easy_setopt(curl_handle, CURLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */ 
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, payload_source);

    upload_context_t context = {
        .bytes_read = 0,
        .message = mail_payload
    };
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, &context);
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);

    /* get it! */ 
    res = curl_easy_perform(curl_handle);

    /* check for errors */ 
    if(res != CURLE_OK) 
    {
        LOG_ERROR(DT_MAIL, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    }
    else 
    {
        LOG_DEBUG(DT_MAIL, "Mail sent successfully");
    }

    /* Free the list of recipients */ 
    curl_slist_free_all(recipients);

    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl_handle);

    return variant_create_bool(true);
}

variant_t*  mail_get_template(service_method_t* method, va_list args)
{
    variant_t* template_name = va_arg(args, variant_t*);
    
    template_t* template = mail_data_get_template(variant_get_string(template_name));

    return variant_create_string(strdup(template->template));
}

