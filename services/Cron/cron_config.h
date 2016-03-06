extern int DT_CRON;

typedef struct cron_entry_t
{
    int minute;
    int hour;
    int day;
    int month;
    int week_day;
    char* scene;
} cron_entry_t;

extern variant_stack_t* crontab;
