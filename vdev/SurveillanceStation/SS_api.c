#include <curl_util.h>
#include <string.h>
#include "SS_config.h"
#include <crc32.h>
#include <logger.h>
#include <ctype.h>
#include "event.h"
#include "service.h"
#include "base64.h"
#include <math.h>

void  SS_api_get_stm_path(int);
void  SS_api_get_camera_info(int);

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
void process_query_response(const json_object* obj, void* arg);
void SS_api_query()
{
    char query_req_buf[512] = {0};
    snprintf(query_req_buf, 511, "%s/webapi/query.cgi?api=SYNO.API.Info&method=Query&version=1&query=SYNO.API.Auth,SYNO.SurveillanceStation.Event,SYNO.SurveillanceStation.Camera,SYNO.SurveillanceStation.Info", SS_base_url);
    curl_util_get_json(query_req_buf, process_query_response, NULL);
}

void process_query_response(const json_object* obj, void* arg)
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
                        //json_object_put(auth_path_data);
                    }

                    //json_object_put(auth_data);
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
                        //json_object_put(event_path_data);
                    }

                    //json_object_put(event_data);
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
                        //json_object_put(camera_path_data);
                    }

                    //json_object_put(camera_data);
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
                        //json_object_put(info_path_data);
                    }

                    //json_object_put(info_data);
                }

                //json_object_put(query_data_array);
            }
        }

        //json_object_put(success_response);
    }
}

// http://192.168.1.77:5000/webapi/auth.cgi?api=SYNO.API.Auth&method=Login&version=2&account=XXXXX&passwd=XXXXX&session=SurveillanceStation&format=sid
// Reply: {"data":  {"sid":"FFWEaNWGizqE.BCK3N02417"},
//         "success":true
//        }
void process_auth_response(const json_object* obj, void* arg);
void SS_api_get_sid()
{
    char auth_req_buf[512] = {0};
    snprintf(auth_req_buf, 511, "%s/webapi/%s?api=SYNO.API.Auth&method=Login&version=2&account=%s&passwd=%s&session=SurveillanceStation&format=sid", SS_base_url, SS_auth_path, SS_user, SS_pass);
    curl_util_get_json(auth_req_buf, process_auth_response, NULL);
}

void process_auth_response(const json_object* obj, void* arg)
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
                    //json_object_put(sid_value);
                }
            }
            //json_object_put(sid_object);
        }

        //json_object_put(success_response);
    }
}

// Logout:
//  GET /webapi/auth.cgi?api=SYNO.API.Auth&method=Logout&version=2&session=SurveillanceStation&_sid=\u201dJn5dZ9aS95wh2\u201d
//
void  SS_api_logout()
{
    char logout_req_buf[512] = {0};
    snprintf(logout_req_buf, 511, "%s/webapi/%s?api=SYNO.API.Auth&method=Logout&version=2&session=SurveillanceStation&_sid=%s", SS_base_url, SS_auth_path, SS_auth_sid);
    curl_util_get_json(logout_req_buf, NULL, NULL);

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
void    process_get_info_response(const json_object* obj, void* arg);
void  SS_api_get_info()
{
    LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Get System List");
    char get_info_req_buf[512] = {0};
    snprintf(get_info_req_buf, 511, "%s/webapi/%s?api=SYNO.SurveillanceStation.Info&method=GetInfo&version=1&_sid=%s", SS_base_url, SS_info_path, SS_auth_sid);
    curl_util_get_json(get_info_req_buf, process_get_info_response, NULL);
}

void    process_get_info_response(const json_object* obj, void* arg)
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
                    //json_object_put(camera_num);
                }
                //json_object_put(data_object);
            }
        }

        //json_object_put(success_response);
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
void process_motion_events_response(const json_object* obj, void* arg);

void get_camera_id(SS_camera_info_t* cam_info, SS_event_keeper_t* ss_event)
{
    //printf("%s looking for cam  = %s, compare with %s\n", __FUNCTION__, cam_info->name, ss_event->camera_name);

    char* trimmed_name = ss_event->camera_name;
    int index = 0;

    while(isdigit(trimmed_name[index]))
    {
        index++;
    }

    // Skip "-" sign
    index++;
    trimmed_name = ss_event->camera_name + index;

    if(strcmp(cam_info->name, trimmed_name) == 0)
    {
        //printf("%s found cam id = %d\n", __FUNCTION__, cam_info->id);
        ss_event->camera_id = cam_info->id;
    }
}

void  SS_api_get_motion_events()
{
    char motion_events_req_buf[512] = {0};
    snprintf(motion_events_req_buf, 511, "%s/webapi/%s?api=SYNO.SurveillanceStation.Event&method=CountByCategory&reason=2,7&fromTime=%d&version=4&_sid=%s", SS_base_url, SS_event_path, time(NULL) - QUERY_RATE_SEC, SS_auth_sid);
    curl_util_get_json(motion_events_req_buf, process_motion_events_response, NULL);
}

void process_motion_events_response(const json_object* obj, void* arg)
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
                                    //printf("Inserting motion event with cam_name %s\n", cam_name);
                                    int event_count = json_object_get_int(event_count_object);
                                    uint32_t crc = crc32(0, cam_name, strlen(cam_name));
                                    variant_t* ss_event_variant = variant_hash_get(SS_event_keeper_table, crc);
                                    SS_event_keeper_t* ss_event = NULL;

                                    if(NULL == ss_event_variant)
                                    {
                                        ss_event = calloc(1, sizeof(SS_event_keeper_t));
                                        ss_event->camera_name = strdup(cam_name);
                                        ss_event->event_count = event_count;
                                        ss_event->old_event_count = 0;
                                        ss_event->events_info_stack = stack_create();
                                        variant_hash_insert(SS_event_keeper_table, crc, variant_create_ptr(DT_PTR, ss_event, NULL));
                                    }
                                    else
                                    {
                                        ss_event = (SS_event_keeper_t*)variant_get_ptr(ss_event_variant);
                                        ss_event->event_count = event_count;
                                    }

                                    if(NULL != ss_event && ss_event->camera_id == 0)
                                    {
                                        // Lets find the camera ID:
                                        variant_hash_for_each_value(SS_camera_info_table, SS_camera_info_t*, get_camera_id, ss_event);
                                        //printf("%s CAMERA_ID = %d\n", __FUNCTION__, ss_event->camera_id);
                                    }

                                }
                            }
                        }
                    }
                    //json_object_put(evt_cam_object);
                }
                //json_object_put(data_object);
            }
        }

        //json_object_put(success_response);
    }
}

/*
{  
   "data":{  
      "events":[  
         {  
            "archived":false,
            "archived_folder":"Family Room",
            "audioCodec":"",
            "audio_format":"",
            "camIdOnRecServer":0,
            "cameraId":1,
            "camera_name":"Family Room",
            "closing":false,
            "deleted":false,
            "dsId":0,
            "eventId":26234,
            "eventSize":6.003118515014648,
            "event_size_bytes":6294726,
            "fisheye_origin_view":false,
            "fisheye_type":{  

            },
            "folder":"/volume1/surveillance/Family Room",
            "for_rotation_only":false,
            "frameCount":154,
            "id":26234,
            "idOnRecServer":0,
            "imageEnhancement":{  
               "brightness":0,
               "contrast":0,
               "saturation":0,
               "sharpness":0
            },
            "imgHeight":480,
            "imgWidth":640,
            "is_complete":true,
            "markAsDel":false,
            "mode":1,
            "mountId":0,
            "mountSrcDsId":0,
            "mount_type":0,
            "mute":false,
            "name":"20170307AM/Family Room20170307-084321-1488897801.mp4",
            "path":"20170307AM/Family Room20170307-084321-1488897801.mp4",
            "reason":2,
            "recordId":"0_26234",
            "recording":false,
            "resoH":480,
            "resoW":640,
            "snapshot_medium":"/9j/4AAQSkZJRgABAQAAAQABARo1VhnYLtzx9aK7Iy5lc45Lldj/2Q==",
            "startTime":1488897801,
            "status":0,
            "status_flags":0,
            "stopTime":1488897821,
            "update_time":3273,
            "videoCodec":"MJPEG",
            "video_type":1,
            "volume":0
         }
      ],
      "offset":0,
      "timestamp":"1488898419",
      "total":1
   },
   "success":true
}*/

void delete_ss_event_info(void* arg)
{
    SS_event_info_t* e = (SS_event_info_t*)arg;
    free(e->path);
    free(e->snapshot);
    free(e);
}

void process_events_info_response(const json_object* obj, void* arg);
void  SS_api_get_events_info(SS_event_keeper_t* ev)
{
    char events_info_req_buf[512] = {0};
    snprintf(events_info_req_buf, 511, "%s/webapi/%s?api=SYNO.SurveillanceStation.Event&method=List&version=4&cameraIds=%d&fromTime=%d&locked=0&evtSrcType=2&blIncludeSnapshot=true&_sid=%s", SS_base_url, SS_event_path, ev->camera_id, time(NULL) - QUERY_RATE_SEC, SS_auth_sid);
    curl_util_get_json(events_info_req_buf, process_events_info_response, (void*)ev);
}

void process_events_info_response(const json_object* obj, void* arg)
{
    struct json_object* success_response;
    json_object_object_get_ex(obj, "success", &success_response);
    if(NULL != success_response)
    {
        if(TRUE == json_object_get_boolean(success_response))
        {
            //printf("process_events_info_response - success\n");
            struct json_object* data_object;
            if(TRUE == json_object_object_get_ex(obj, "data", &data_object))
            {
                struct json_object* events_array;
                if(TRUE == json_object_object_get_ex(data_object, "events", &events_array))
                {
                    SS_event_keeper_t* ss_event = (SS_event_keeper_t*)(arg);
                    //stack_empty(ss_event->events_info_stack);
                    struct json_object* event_entry;
                    for (int i = 0; i < json_object_array_length(events_array); i++) 
                    {
                        event_entry = json_object_array_get_idx(events_array, i);
                        SS_event_info_t* ss_event_info = calloc(1, sizeof(SS_event_info_t));

                        struct json_object* event_id_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "eventId", &event_id_obj))
                        {
                            ss_event_info->event_id = json_object_get_int(event_id_obj);
                            //printf("process_events_info_response - got event id %d\n", ss_event_info->event_id);
                        }

                        struct json_object* event_size_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "event_size_bytes", &event_size_obj))
                        {
                            ss_event_info->event_size_bytes = json_object_get_int(event_size_obj);
                        }

                        struct json_object* event_path_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "path", &event_path_obj))
                        {
                            ss_event_info->path = strdup(json_object_get_string(event_path_obj));
                        }

                        struct json_object* event_snapshot_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "snapshot_medium", &event_snapshot_obj))
                        {
                            ss_event_info->snapshot = strdup(json_object_get_string(event_snapshot_obj));
                        }
                         
                        struct json_object* event_height_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "imgHeight", &event_height_obj))
                        {
                            ss_event_info->imgHeight = json_object_get_int(event_height_obj);
                        }

                        struct json_object* event_width_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "imgWidth", &event_width_obj))
                        {
                            ss_event_info->imgWidth = json_object_get_int(event_width_obj);
                        }

                        struct json_object* event_start_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "startTime", &event_start_obj))
                        {
                            ss_event_info->start_time = json_object_get_int(event_start_obj);
                        }

                        struct json_object* event_stop_obj;
                        if(TRUE == json_object_object_get_ex(event_entry, "stopTime", &event_stop_obj))
                        {
                            ss_event_info->stop_time = json_object_get_int(event_stop_obj);
                        }

                        stack_push_back(ss_event->events_info_stack, variant_create_ptr(DT_PTR, ss_event_info, &delete_ss_event_info));
                    }
                }
            }
        }
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
void process_camera_list_response(const json_object* obj, void* arg);
void  SS_api_get_camera_list()
{
    LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Get Camera List");
    char camera_list_req_buf[512] = {0};
    snprintf(camera_list_req_buf, 511, "%s/webapi/%s?privCamType=3&version=8&streamInfo=true&blPrivilege=false&start=0&api=SYNO.SurveillanceStation.Camera&basic=true&blFromCamList=true&camStm=1&method=List&_sid=%s", SS_base_url, SS_camera_path, SS_auth_sid);
    curl_util_get_json(camera_list_req_buf, process_camera_list_response, NULL);
}

void process_camera_list_response(const json_object* obj, void* arg)
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
                            //json_object_put(camera_id_object);
                        }

                        const char* snapshot_path;
                        struct json_object* snapthot_path_obj;
                        if(TRUE == json_object_object_get_ex(camera_entry, "snapshot_path", &snapthot_path_obj))
                        {
                            snapshot_path = json_object_get_string(snapthot_path_obj);
                        }

                        struct json_object* stm_info_array;
                        int num_streams = 0;
                        int max_fps = 0;
                        int max_width = 0;
                        int max_height = 0;
                        if(TRUE == json_object_object_get_ex(camera_entry, "stm_info", &stm_info_array))
                        {
                            num_streams = json_object_array_length(stm_info_array);
                            for (int i = 0; i < num_streams; i++)
                            {
                                struct json_object* stm_entry = json_object_array_get_idx(stm_info_array, i);
                                
                                struct json_object* fps_obj;
                                json_object_object_get_ex(stm_entry, "fps", &fps_obj);
                                int fps = json_object_get_int(fps_obj);
                                if(max_fps < fps) max_fps = fps;

                                struct json_object* resolution_obj;
                                json_object_object_get_ex(stm_entry, "resolution", &resolution_obj);
                                const char* resolution = json_object_get_string(resolution_obj);

                                // Now split the WxH into 2 ints
                                variant_stack_t* width_height = create_cmd_vec_with_separator(resolution, 'x');
                                if(width_height->count == 2)
                                {
                                    variant_t* width_var = stack_pop_front(width_height);
                                    variant_t* height_var = stack_pop_front(width_height);
                                    stack_free(width_height);
                                    int width = atoi(variant_get_string(width_var));
                                    int height = atoi(variant_get_string(height_var));
                                    variant_free(width_var);
                                    variant_free(height_var);
                                    if(max_width < width) max_width = width;
                                    if(max_height < height) max_height = height;
                                }
                            }
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
                                cam_info->snapshot_path = strdup(snapshot_path);
                                cam_info->max_fps = max_fps;
                                cam_info->max_width = max_width;
                                cam_info->max_height = max_height;
                                cam_info->num_streams = num_streams;
                                LOG_DEBUG(DT_SURVEILLANCE_STATION, "Found camera %d: %s", cam_info->id, cam_info->name);
                                variant_hash_insert(SS_camera_info_table, cam_id, variant_create_ptr(DT_PTR, cam_info, NULL));
                                
                                // Get STM URL for each cam ID
                                SS_api_get_stm_path(cam_id);
                                SS_api_get_camera_info(cam_id);
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

                            //json_object_put(camera_name_object);
                        }
                    }
                    //json_object_put(camera_array);
                }
                //json_object_put(data_object);
            }
        }

        //json_object_put(success_response);
    }
}

// Http://192.168.1.1:5000/webapi/entry.cgi?api=SYNO.SurveillanceStation.Camera&method=GetStmUrlPath&version=8&cameraIds=”14,15”
void process_camera_stm_response(const json_object* obj, void* arg);
void  SS_api_get_stm_path(int cam_id)
{
    LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Get Camera Stream URL path");
    char camera_list_req_buf[512] = {0};
    snprintf(camera_list_req_buf, 511, "%s/webapi/%s?version=8&api=SYNO.SurveillanceStation.Camera&cameraIds=%d&method=GetStmUrlPath&_sid=%s", SS_base_url, SS_camera_path, cam_id, SS_auth_sid);
    curl_util_get_json(camera_list_req_buf, process_camera_stm_response, (void*)cam_id);
}

void process_camera_stm_response(const json_object* obj, void* arg)
{
    int cam_id = (int)arg;
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
                if(TRUE == json_object_object_get_ex(data_object, "pathInfos", &camera_array))
                {
                    // Only one entry at a time...
                    struct json_object* camera_entry = json_object_array_get_idx(camera_array, 0);
                    const char* stm_path;
                    struct json_object* camera_stm_path_object;
                    if(TRUE == json_object_object_get_ex(camera_entry, "unicastPath", &camera_stm_path_object))
                    {
                        stm_path = json_object_get_string(camera_stm_path_object);

                        variant_t* cam_entry_variant = variant_hash_get(SS_camera_info_table, cam_id);
                        if(NULL != cam_entry_variant)
                        {
                            SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_entry_variant);
                            free(cam_info->stm_url_path);
                            cam_info->stm_url_path = strdup(stm_path);
                        }
                    }
                }
            }
        }
    }
}

//http://192.168.1.1:5000/webapi/entry.cgi?
//version="8"&cameraIds="89"&blIncludeDeletedCam=true&deviceOutCap=true&streamInfo=true&method
//="GetInfo"&api="SYNO.SurveillanceStation.Camera"&ptz=true&basic=true&privCamType=3&camAppInfo=t
//rue&optimize=true&fisheye=true&eventDetection=true
void process_camera_info_response(const json_object* obj, void* arg);
void  SS_api_get_camera_info(int cam_id)
{
    LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Get Camera Info");
    char camera_info_req_buf[512] = {0};
    snprintf(camera_info_req_buf, 511, "%s/webapi/%s?api=SYNO.SurveillanceStation.Camera&version=8&cameraIds=%d&basic=true&camAppInfo=true&method=GetInfo&_sid=%s", SS_base_url, SS_camera_path, cam_id, SS_auth_sid);
    LOG_DEBUG(DT_SURVEILLANCE_STATION, "Get Camera Info URL: %s", camera_info_req_buf);

    curl_util_get_json(camera_info_req_buf, process_camera_info_response, (void*)cam_id);
}

void process_camera_info_response(const json_object* obj, void* arg)
{
    int cam_id = (int)arg;
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
                    variant_t* cam_entry_variant = variant_hash_get(SS_camera_info_table, cam_id);
                    if(NULL != cam_entry_variant)
                    {
                        SS_camera_info_t* cam_info = (SS_camera_info_t*)variant_get_ptr(cam_entry_variant);

                        struct json_object* camera_entry = json_object_array_get_idx(camera_array, 0);
                        struct json_object* host_object;
                        if(TRUE == json_object_object_get_ex(camera_entry, "host", &host_object))
                        {
                            cam_info->host = strdup(json_object_get_string(host_object));
                        }

                        struct json_object* port_object;
                        if(TRUE == json_object_object_get_ex(camera_entry, "port", &port_object))
                        {
                            cam_info->port = json_object_get_int(port_object);
                        }

                        struct json_object* detail_info;
                        if(TRUE == json_object_object_get_ex(camera_entry, "detailInfo", &detail_info))
                        {
                            struct json_object* username_object;
                            if(TRUE == json_object_object_get_ex(detail_info, "camUserName", &username_object))
                            {
                                cam_info->username = strdup(json_object_get_string(username_object));
                            }

                            struct json_object* password_object;
                            if(TRUE == json_object_object_get_ex(detail_info, "camPassWord", &password_object))
                            {
                                cam_info->password = strdup(json_object_get_string(password_object));
                            }
                        }
                    }
                }
            }
        }
    }
}

// http://192.168.1.1:5000/webapi/entry.cgi?camStm=1&version="8"&cameraId=18&api="SYNO.SurveillanceStation.Camera"&preview=true&method="GetSnapshot"
void process_camera_snapshot_response(const char* data, int len, void* arg);
void SS_api_get_camera_snapshot(int cam_id, char** snapshot)
{
    LOG_ADVANCED(DT_SURVEILLANCE_STATION, "Get Camera Snapshot");
    char camera_snapshot_req_buf[512] = {0};
    snprintf(camera_snapshot_req_buf, 511, "%s/webapi/%s?api=SYNO.SurveillanceStation.Camera&version=8&cameraId=%d&camStm=1&method=GetSnapshot&_sid=%s", SS_base_url, SS_camera_path, cam_id, SS_auth_sid);
    LOG_DEBUG(DT_SURVEILLANCE_STATION, camera_snapshot_req_buf);
    curl_util_get_resource(camera_snapshot_req_buf, process_camera_snapshot_response, (void*)snapshot);
}

void process_camera_snapshot_response(const char* data, int len, void* arg)
{
    char** snapshot = (char**)arg;
    *snapshot = calloc(ceil((float)len/3.0f)*5.0f, sizeof(char));

    Base64encode(*snapshot, data, len);
    LOG_DEBUG(DT_SURVEILLANCE_STATION, "Snapshot: %s", *snapshot);
}

