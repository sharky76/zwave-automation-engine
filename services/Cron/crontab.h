#include <stack.h>
#include <variant.h>

typedef struct crontab_time_entry_t
{
    variant_stack_t* time_list;
    variant_stack_t*    child_array;
} crontab_time_entry_t;

typedef variant_stack_t* crontab_time_t[5];
typedef int current_time_value_t[5];

typedef enum CronAction
{
    CA_SCENE,
    CA_COMMAND
} CronAction;

typedef struct cron_action_t
{
    CronAction type;
    variant_t* command;
} cron_action_t;

#define ADD_MINUTE(cron_arr, val)       cron_arr[0] = val
#define ADD_HOUR(cron_arr, val)         cron_arr[1] = val
#define ADD_DAY(cron_arr, val)          cron_arr[2] = val
#define ADD_MONTH(cron_arr, val)        cron_arr[3] = val
#define ADD_WEEKDAY(cron_arr, val)      cron_arr[4] = val


#define MINUTE(cron_arr)    cron_arr[0]
#define HOUR(cron_arr)      cron_arr[1]
#define DAY(cron_arr)       cron_arr[2]
#define MONTH(cron_arr)     cron_arr[3]
#define WEEKDAY(cron_arr)   cron_arr[4]


void    crontab_add_entry(crontab_time_t crontab_entry, CronAction actionType, char* command);
void    crontab_del_entry(crontab_time_t crontab_entry, CronAction actionType, char* command);
variant_stack_t*  crontab_get_action(current_time_value_t crontab_time);
bool    crontab_time_compare(crontab_time_t left, crontab_time_t right);


