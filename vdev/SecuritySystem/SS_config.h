extern int  DT_SECURITY_SYSTEM;

typedef enum SecuritySystemState_t
{
    STAY_ARMED = 0,
    AWAY_ARM = 1,
    NIGHT_ARM = 2,
    DISARMED = 3,
    ALARM_TRIGGERED = 4
} SecuritySystemState_t;

extern SecuritySystemState_t    SS_State;

#define COMMAND_CLASS_SECURITY_SYSTEM 0xF1     // Special command class for this virtual device
#define COMMAND_CLASS_MODEL_INFO 114
