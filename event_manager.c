#include "event_manager.h"
#include <event.h>
#include "scene_manager.h"
#include "service_manager.h"
#include <signal.h>
#include "logger.h"

extern variant_stack_t* event_list;
void event_manager_handle_event(int sig);

void event_manager_init()
{
    LOG_ADVANCED("Initializing event manager");
    event_list = stack_create();

    // Setup SIGALARM handler
    struct sigaction sa;
    sa.sa_handler = &event_manager_handle_event;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, 0) == -1) 
    {
        LOG_ERROR("Error initializing event manager: fail to setup signal handler");
    }

    LOG_ADVANCED("Event manager initialized");

}

void event_manager_handle_event(int sig)
{
    LOG_DEBUG("Event received");
    event_t* event = event_receive();
    scene_manager_on_event(event);
    service_manager_on_event(event);
    event_delete(event);
}
