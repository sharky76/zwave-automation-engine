#include <stdbool.h>

int homebridge_manager_init();
void homebridge_manager_set_start_state(bool isStart);
void homebridge_manager_stop();
bool homebridge_manager_is_running();