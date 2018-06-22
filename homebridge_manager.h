#include <stdbool.h>

int homebridge_manager_init();
void homebridge_manager_stop();
bool homebridge_manager_is_running();
void homebridge_on_event(int homebridge_fd, void* context);
