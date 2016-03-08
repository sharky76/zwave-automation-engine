#include <stack.h>
#include <variant.h>

typedef struct crontab_time_entry_t
{
    int time;
    variant_stack_t*    child_array;
} crontab_time_entry_t;

typedef int crontab_time_t[5];
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

void    crontab_add_entry(crontab_time_t crontab_entry, char* scene);
void    crontab_del_entry(crontab_time_t crontab_entry, char* scene);
variant_stack_t*  crontab_get_scene(crontab_time_t crontab_time);


