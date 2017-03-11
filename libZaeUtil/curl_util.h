#include <curl/curl.h>
#include <json-c/json.h>
#include <stdbool.h>

bool    curl_util_get_json(const char* request_url, void (response_parser)(const json_object*, void*), void* arg);
