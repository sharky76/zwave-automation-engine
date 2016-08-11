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

// http://192.168.1.77:5000/webapi/auth.cgi?api=SYNO.API.Auth&method=Login&version=2&account=XXXXX&passwd=XXXXX&session=SurveillanceStation&format=sid
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

// Logout:
//  GET /webapi/auth.cgi?api=SYNO.API.Auth&method=Logout&version=2&session=SurveillanceStation&_sid=\u201dJn5dZ9aS95wh2\u201d
//
void  SS_api_logout()
{
    char logout_req_buf[512] = {0};
    snprintf(logout_req_buf, 511, "%s/%s?api=SYNO.API.Auth&method=Logout&version=2&session=SurveillanceStation&_sid=%s", SS_base_url, SS_auth_path, SS_auth_sid);
    curl_util_get_json(logout_req_buf, NULL);

    free(SS_auth_sid);
    SS_auth_sid = NULL;
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

// Get Camera List
// http://192.168.1.77:5000/webapi/_______________________________________________________entry.cgi?privCamType=3&version=%228%22&streamInfo=true&blPrivilege=false&start=0&api=%22SYNO.SurveillanceStation.Camera%22&limit=2&basic=true&blFromCamList=true&camStm=1&method=%22List%22&_sid=OPH7uhlet91U.BCK3N02417
// 
// Response:
// {"data": {"cameras": [{"DINum":0,
//                        "DONum":0,
//                        "audioCap":false,
//                        "audioOut":false,
//                        "audioType":0,
//                        "blAudioDisableRec":false,
//                        "blAudioPriv":true,
//                        "blDisableRec":false,
//                        "blEnableExtDI":false,
//                        "blLiveviewPriv":true,
//                        "blReceivePocZero":false,
//                        "camFov":"",
//                        "camIdOnRecServer":0,
//                        "camLiveMode":1,
//                        "camMobileLiveMode":0,
//                        "camMountType":0,
//                        "camPath":"aHR0cDovL2FkbWluOnF3ZXJ0eUAxOTIuMTY4LjEuODg6ODAvdmlkZW9zdHJlYW0uY2dpP3JhdGU9MA==",
//                        "camRecShare":"surveillance",
//                        "camRecStorageStatus":0,
//                        "camRecVolume":"volume1",
//                        "camRotOption":0,
//                        "camStatus":1,
//                        "camVideoType":"MJPEG",
//                        "channel_id":"1",
//                        "deleted":false,
//                        "deviceType":4,
//                        "dsIp":"",
//                        "dsPort":5000,
//                        "enabled":true,
//                        "extDIDev":0,
//                        "extDIPorts":0,
//                        "folder":"/volume1/surveillance/Family Room",
//                        "forceMjpeg":false,
//                        "fps":20,
//                        "hasCamParam":true,
//                        "host":"192.168.1.88",
//                        "id":1,
//                        "isStatusUnrecognized":false,
//                        "is_rotated_by_date":true,
//                        "is_rotated_by_space":true,
//                        "model":"FI8910W",
//                        "name":"Family Room",
//                        "ownerDsId":0,
//                        "port":80,
//                        "presetNum":8,
//                        "privilege":15,
//                        "ptzCap":11,"quality":"","recBitrateCtrl":0,"recCbrBitrate":1000,"recStatus":0,"resolution":"640x480","rotation_by_date":30,"rotation_by_space":"10","rotation_option":0,"singleStream":false,"snapshot_path":"/webapi/_______________________________________________________entry.cgi?api=SYNO.SurveillanceStation.Camera&method=GetSnapshot&version=1&cameraId=1&timestamp=1462711434&preview=true&camStm=1","status":0,"status_flags":0,"stmFisheyeType":0,"stm_info":[{"fps":20,"quality":"","resolution":"640x480","type":0},{"fps":20,"quality":"","resolution":"640x480","type":1},{"fps":20,"quality":"","resolution":"640x480","type":2}],"tvStandard":0,"type":1,"uiStmNoList":"1,1,1","update_time":1459034407,"vendor":"FOSCAM","videoCapList":[{"stList":["HTTP"],"vt":"MJPEG"}],"volume_space":"9.988"}],
//          "delcam":[],
//          "existCamMntTypeMap":null,
//          "keyUsedCnt":1,
//          "timestamp":"1462711434",
//          "total":1},
// "success":true}
void process_camera_list_response(const json_object* obj);
void  SS_api_get_camera_list()
{
    char camera_list_req_buf[512] = {0};
    snprintf(camera_list_req_buf, 511, "%s/%s?privCamType=3&version=8&streamInfo=true&blPrivilege=false&start=0&api=SYNO.SurveillanceStation.Camera&basic=true&blFromCamList=true&camStm=1&method=List&_sid=%s", SS_base_url, SS_camera_path, SS_auth_sid);
    curl_util_get_json(camera_list_req_buf, process_camera_list_response);
}

void process_camera_list_response(const json_object* obj)
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
                struct json_object* camera_array;
                if(TRUE == json_object_object_get_ex(data_object, "cameras", &camera_array))
                {
                    struct json_object* camera_entry;
                    int cam_id;
                    for (int i = 0; i < json_object_array_length(camera_array); i++) 
                    {
                        camera_entry = json_object_array_get_idx(camera_array, i);
                        
                        struct json_object* camera_id_object;
                        if(TRUE == json_object_object_get_ex(camera_entry, "id", &camera_id_object))
                        {
                            cam_id = json_object_get_int(camera_id_object);
                            json_object_put(camera_id_object);
                        }

                        struct json_object* camera_name_object;
                        if(TRUE == json_object_object_get_ex(camera_entry, "name", &camera_name_object))
                        {
                            // Add or update camera list table
                            variant_t* cam_entry_variant = variant_hash_get(SS_camera_info_table, cam_id);
                            if(NULL == cam_entry_variant)
                            {
                                SS_camera_info_t* cam_info = calloc(1, sizeof(SS_camera_info_t));
                                cam_info->id = cam_id;
                                cam_info->name = strdup(json_object_get_string(camera_name_object));
                                variant_hash_insert(SS_camera_info_table, cam_id, variant_create_ptr(DT_PTR, cam_info, NULL));
                            }
                            else
                            {
                                SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_entry_variant);
                                if(strcmp(cam_info->name, json_object_get_string(camera_name_object)))
                                {
                                    free(cam_info->name);
                                    cam_info->name = strdup(json_object_get_string(camera_name_object));
                                }
                            }

                            json_object_put(camera_name_object);
                        }
                    }
                    json_object_put(camera_array);
                }
                json_object_put(data_object);
            }
        }

        json_object_put(success_response);
    }
}