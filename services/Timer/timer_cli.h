#include <cli.h>

typedef struct timer_config_t
{
    bool singleshot;
    int  timeout;
    int  event_id;
    char* name;
} timer_config_t;

void    timer_cli_init(cli_node_t* parent_node);
char**  timer_cli_get_config(vty_t* vty);
