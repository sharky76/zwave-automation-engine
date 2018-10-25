#include <hash.h>
#include <stack.h>

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

typedef struct SS_event_info_t
{
    int event_id;
    int event_size_bytes;
    int imgHeight;
    int imgWidth;
    int start_time;
    int stop_time;
    char* path;
    char* snapshot;
} SS_event_info_t;

typedef struct SS_event_keeper_t
{
    int     camera_id;
    char*   camera_name;
    int     event_count;
    int     old_event_count;
    bool    event_active;
    variant_stack_t*    events_info_stack;
    int     active_event_tick_counter;
} SS_event_keeper_t;

extern hash_table_t* SS_event_keeper_table;

typedef struct SS_camera_info_t
{
    int     id;
    char*   name;
    char*   snapshot_path;
    char*   stm_url_path;
    int     num_streams;
    int     max_fps;
    int     max_width;
    int     max_height;
} SS_camera_info_t;

extern hash_table_t* SS_camera_info_table;

extern bool    SS_device_started;

#define QUERY_RATE_SEC 30
#define EVENT_ACTIVE_TIMEOUT_SEC 180
#define COMMAND_CLASS_MOTION_EVENTS 48
#define COMMAND_CLASS_MODEL_INFO 114
#define COMMAND_CLASS_CAMERA 0xF2     // Special command class for this virtual device

