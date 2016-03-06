#include "cron_cli.h"

bool cmd_cron_add_entry(vty_t* vty, variant_stack_t* params);
bool cmd_cron_del_entry(vty_t* vty, variant_stack_t* params);

cli_node_t* cron_node;

cli_command_t cron_command_list[] = {
    {"execute *|INT[0-59] *|INT[0-23] *|INT[1-31] *|INT[1-12] *|INT[0-7] scene LINE",   cmd_cron_add_entry, "Add cron entry: min[0-59] hour[0-23] day of month[1-31] month[1-12] day of week[0-7] (0 = Monday). * means first-last"},
    {"no execute *|INT[0-59] *|INT[0-23] *|INT[1-31] *|INT[1-12] *|INT[0-7] scene LINE",   cmd_cron_del_entry, "Add cron entry: min[0-59] hour[0-23] day of month[1-31] month[1-12] day of week[0-7] (0 = Monday). * means first-last"},
};

void cron_cli_init(cli_node_t* parent_node)
{
    cli_install_node(&cron_node, parent_node, cron_command_list, "Cron", "service-cron");
}

bool cmd_cron_add_entry(vty_t* vty, variant_stack_t* params)
{

}

bool cmd_cron_del_entry(vty_t* vty, variant_stack_t* params)
{

}

