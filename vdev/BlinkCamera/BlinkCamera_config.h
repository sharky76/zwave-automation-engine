#include "stack.h"
#include "vdev.h"

int  DT_BLINK_CAMERA;

typedef struct blink_camera_entry_t
{
    char* name;
    int   instance;
    int   active_event_tick_counter;
    bool  camera_motion_detected_event;
    int   timer_id;
} blink_camera_entry_t;

variant_stack_t* blink_camera_list;

blink_camera_entry_t*    blink_camera_add(int instance, const char* name);
bool                     blink_camera_del(int instance);
blink_camera_entry_t*    blink_camera_find_instance(int instance);
blink_camera_entry_t*    blink_camera_find_name(const char* name);

