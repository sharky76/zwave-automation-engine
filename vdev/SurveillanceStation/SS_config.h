#include <hash.h>

/**
 * SurveillanceStation configuration
 */

extern int  DT_SURVEILLANCE_STATION;

extern char* SS_user;
extern char* SS_pass;
extern char* SS_base_url;
extern char* SS_auth_sid;
extern char* SS_auth_path;
extern char* SS_event_path;
extern char* SS_camera_path;
extern char* SS_info_path;
extern int   SS_camera_count;

typedef struct SS_event_keeper_t
{
    char*   camera_name;
    int     event_count;
    int     old_event_count;
} SS_event_keeper_t;

extern hash_table_t* SS_event_keeper_table;

typedef struct SS_camera_info_t
{
    int     id;
    char*   name;
} SS_camera_info_t;

extern hash_table_t* SS_camera_info_table;

extern bool    SS_device_started;

#define QUERY_RATE_SEC 30
#define COMMAND_CLASS_MOTION_EVENTS 1