#include <curl_util.h>
#include <string.h>
#include "SS_config.h"
#include <crc32.h>
#include <logger.h>

// API Query: http://192.168.1.77:5000/webapi/query.cgi?api=SYNO.API.Info&method=Query&version=1&query=SYNO.API.Auth,SYNO.SurveillanceStation.Event,SYNO.SurveillanceStation.Camera,SYNO.SurveillanceStation.Info
// Reply: 
// {"data": {"SYNO.API.Auth": {"maxVersion":4,
//                             "minVersion":1,
//                             "path":"auth.cgi"},
//           "SYNO.SurveillanceStation.Event": {"maxVersion":5,
//                                              "minVersion":1,
//                                              "path":"_______________________________________________________entry.cgi",
//                                              "requestFormat":"JSON"}},
//  "success":true}
void process_query_response(const json_object* obj);
void SS_api_query()
{
    char query_req_buf[512] = {0};
    snprintf(query_req_buf, 511, "%s/query.cgi?api=SYNO.API.Info&method=Query&version=1&query=SYNO.API.Auth,SYNO.SurveillanceStation.Event,SYNO.SurveillanceStation.Camera,SYNO.SurveillanceStation.Info", SS_base_url);
    curl_util_get_json(query_req_buf, process_query_response);
}

void process_query_response(const json_object* obj)
{
    struct json_object* success_response;
    json_object_object_get_ex(obj, "success", &success_response);
    if(NULL != success_response)
    {
        if(TRUE == json_object_get_boolean(success_response))
        {
            struct json_object* query_data_array; 
            json_object_object_get_ex(obj, "data", &query_data_array);
            if(NULL != query_data_array)
            {
                struct json_object* auth_data;
                json_object_object_get_ex(query_data_array, "SYNO.API.Auth", &auth_data);
                if(NULL != auth_data)
                {
                    struct json_object* auth_path_data;
                    json_object_object_get_ex(auth_data, "path", &auth_path_data);
                    if(NULL != auth_path_data)
                    {
                        free(SS_auth_path);
                        SS_auth_path = strdup(json_object_get_string(auth_path_data));
                        LOG_DEBUG(DT_SURVEILLANCE_STATION, "SYNO.Api.Auth path = %s", SS_auth_path);
                        json_object_put(auth_path_data);
                    }

                    json_object_put(auth_data);
                }

                struct json_object* event_data;
                json_object_object_get_ex(query_data_array, "SYNO.SurveillanceStation.Event", &event_data);
                if(NULL != event_data)
                {
                    struct json_object* event_path_data;
                    json_object_object_get_ex(event_data, "path", &event_path_data);
                    if(NULL != event_path_data)
                    {
                        free(SS_event_path);
                        SS_event_path = strdup(json_object_get_string(event_path_data));
                        LOG_DEBUG(DT_SURVEILLANCE_STATION, "SYNO.SurveillanceStation.Event path = %s", SS_event_path);

                        json_object_put(event_path_data);
                    }

                    json_object_put(event_data);
                }

                struct json_object* camera_data;
                json_object_object_get_ex(query_data_array, "SYNO.SurveillanceStation.Camera", &camera_data);
                if(NULL != camera_data)
                {
                    struct json_object* camera_path_data;
                    json_object_object_get_ex(camera_data, "path", &camera_path_data);
                    if(NULL != camera_path_data)
                    {
                        free(SS_camera_path);
                        SS_camera_path = strdup(json_object_get_string(camera_path_data));
                        LOG_DEBUG(DT_SURVEILLANCE_STATION, "SYNO.SurveillanceStation.Camera path = %s", SS_camera_path);
                        json_object_put(camera_path_data);
                    }

                    json_object_put(camera_data);
                }

                struct json_object* info_data;
                json_object_object_get_ex(query_data_array, "SYNO.SurveillanceStation.Info", &info_data);
                if(NULL != info_data)
                {
                    struct json_object* info_path_data;
                    json_object_object_get_ex(info_data, "path", &info_path_data);
                    if(NULL != info_path_data)
                    {
                        free(SS_info_path);
                        SS_info_path = strdup(json_object_get_string(info_path_data));
                        LOG_DEBUG(DT_SURVEILLANCE_STATION, "SYNO.SurveillanceStation.Info path = %s", SS_info_path);
                        json_object_put(info_path_data);
                    }

                    json_object_put(info_data);
                }

                json_object_put(query_data_array);
            }
        }

        json_object_put(success_response);
    }
}

// http://192.168.1.77:5000/webapi/auth.cgi?api=SYNO.API.Auth&method=Login&version=2&account=XXXX&passwd=XXXXX&session=SurveillanceStation&format=sid
// Reply: {"data":  {"sid":"FFWEaNWGizqE.BCK3N02417"},
//         "success":true
//        }
void process_auth_response(const json_object* obj);
void SS_api_get_sid()
{
    char auth_req_buf[512] = {0};
    snprintf(auth_req_buf, 511, "%s/%s?api=SYNO.API.Auth&method=Login&version=2&account=%s&passwd=%s&session=SurveillanceStation&format=sid", SS_base_url, SS_auth_path, SS_user, SS_pass);
    curl_util_get_json(auth_req_buf, process_auth_response);
}

void process_auth_response(const json_object* obj)
{
    struct json_object* success_response;
    json_object_object_get_ex(obj, "success", &success_response);
    if(NULL != success_response)
    {
        if(TRUE == json_object_get_boolean(success_response))
        {
            struct json_object* sid_object;
            if(TRUE == json_object_object_get_ex(obj, "data", &sid_object))
            {
                struct json_object* sid_value;
                if(TRUE == json_object_object_get_ex(sid_object, "sid", &sid_value))
                {
                    free(SS_auth_sid);
                    SS_auth_sid = strdup(json_object_get_string(sid_value));
                    LOG_DEBUG(DT_SURVEILLANCE_STATION, "Auth SID = %s", SS_auth_sid);
                    json_object_put(sid_value);
                }
            }
            json_object_put(sid_object);
        }

        json_object_put(success_response);
    }
}

// Get Info
//     http://192.168.1.77:5000/webapi/_______________________________________________________entry.cgi?api=SYNO.SurveillanceStation.Info&method=GetInfo&version=1&_sid=6f.rB6ZDs.i9QBCK3N02417
//
//  Reply:
//
//  {"data":    {"cameraNumber":1,
//               "customizedPortHttp":9900,
//               "liscenseNumber":2,
//               "maxCameraSupport":8,
//               "path":"/webman/3rdparty/SurveillanceStation/",
//               "version": {"build":"4141",
//                           "major":"7",
//                           "minor":"1"}
//              },
//   "success":true
//  }
void    process_get_info_response(const json_object* obj);
void  SS_api_get_info()
{
    char get_info_req_buf[512] = {0};
    snprintf(get_info_req_buf, 511, "%s/%s?api=SYNO.SurveillanceStation.Info&method=GetInfo&version=1&_sid=%s", SS_base_url, SS_info_path, SS_auth_sid);
    curl_util_get_json(get_info_req_buf, process_get_info_response);
}

void    process_get_info_response(const json_object* obj)
{
    struct json_object* success_response;
    json_object_object_get_ex(obj, "success", &success_response);
    if(NULL != success_response)
    {
        if(TRUE == json_object_get_boolean(success_response))
        {
            struct json_object* data_object;
            if(TRUE == json_object_object_get_ex(obj, "data", &data_object))
            {
                struct json_object* camera_num;
                if(TRUE == json_object_object_get_ex(data_object, "cameraNumber", &camera_num))
                {
                    SS_camera_count = json_object_get_int(camera_num);
                    json_object_put(camera_num);
                }
                json_object_put(data_object);
            }
        }

        json_object_put(success_response);
    }
}
// http://192.168.1.77:5000/webapi/_______________________________________________________entry.cgi?api=SYNO.SurveillanceStation.Event&method=CountByCategory&reason=2,7&fromTime=1461931670&version=4&_sid=6f.rB6ZDs.i9QBCK3N02417
// Reply:
// {"data": {"date": {"-1":49,
//                    "2016/04/29": {"-1":49,
//                                   "am":0,
//                                   "pm":49}},
//           "evt_cam": {"-1":49,
//                       "0": {"-1":49,
//                             "1-Family Room":49}},
//           "total":49},
//  "success":true}
void process_motion_events_response(const json_object* obj);
void  SS_api_get_motion_events()
{
    char motion_events_req_buf[512] = {0};
    snprintf(motion_events_req_buf, 511, "%s/%s?api=SYNO.SurveillanceStation.Event&method=CountByCategory&reason=2,7&fromTime=%d&version=4&_sid=%s", SS_base_url, SS_event_path, time(NULL) - QUERY_RATE_SEC, SS_auth_sid);
    curl_util_get_json(motion_events_req_buf, process_motion_events_response);
}

void process_motion_events_response(const json_object* obj)
{
    struct json_object* success_response;
    json_object_object_get_ex(obj, "success", &success_response);
    if(NULL != success_response)
    {
        if(TRUE == json_object_get_boolean(success_response))
        {
            struct json_object* data_object;
            if(TRUE == json_object_object_get_ex(obj, "data", &data_object))
            {
                struct json_object* evt_cam_object;
                if(TRUE == json_object_object_get_ex(data_object, "evt_cam", &evt_cam_object))
                {
                    json_object_object_foreach(evt_cam_object, key, value)
                    {
                        if(strcmp(key, "-1"))
                        {
                            // Anything not "-1" is our DS id
                            json_object_object_foreach(value, cam_name, event_count_object)
                            {
                                if(strcmp(cam_name, "-1"))
                                {
                                    int event_count = json_object_get_int(event_count_object);
                                    uint32_t crc = crc32(0, cam_name, strlen(cam_name));
                                    variant_t* ss_event_variant = variant_hash_get(SS_event_keeper_table, crc);

                                    if(NULL == ss_event_variant)
                                    {
                                        SS_event_keeper_t* ss_event = calloc(1, sizeof(SS_event_keeper_t));
                                        ss_event->camera_name = strdup(cam_name);
                                        ss_event->event_count = event_count;
                                        ss_event->old_event_count = 0;
                                        variant_hash_insert(SS_event_keeper_table, crc, variant_create_ptr(DT_PTR, ss_event, NULL));
                                    }
                                    else
                                    {
                                        SS_event_keeper_t* ss_event = (SS_event_keeper_t*)variant_get_ptr(ss_event_variant);
                                        ss_event->event_count = event_count;
                                    }
                                }
                            }
                        }
                    }
                    json_object_put(evt_cam_object);
                }
                json_object_put(data_object);
            }
        }

        json_object_put(success_response);
    }
}

