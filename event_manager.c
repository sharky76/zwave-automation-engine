#include "event_manager.h"
#include <event.h>
#include "scene_manager.h"
#include "service_manager.h"
#include <signal.h>
#include "logger.h"
#include <pthread.h>
#include <semaphore.h>

extern variant_stack_t* event_list;

//int DT_SENSOR;
//int DT_DEV_RECORD;
//int DT_SERVICE;
//int DT_DATA_ENTRY_TYPE;
//int DT_USER_LOCAL_TYPE;

void* event_manager_handle_event(void* arg);
void event_received(int sig);

//pthread_cond_t  event_received_condition;
//pthread_mutex_t event_received_cond_mutex;
sem_t   event_semaphore;

DECLARE_LOGGER(Event)

void event_manager_init()
{
    LOG_ADVANCED(Event, "Initializing event manager");
    event_list = stack_create();

    //DT_SENSOR_EVENT = variant_get_next_user_type();

    //DT_DEV_RECORD = variant_get_next_user_type();
    //DT_SERVICE = variant_get_next_user_type();
    //DT_DATA_ENTRY_TYPE = variant_get_next_user_type();
    //DT_USER_LOCAL_TYPE= variant_get_next_user_type();

    // Setup SIGALARM handler
    struct sigaction sa;
    sa.sa_handler = &event_received;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, 0) == -1) 
    {
        LOG_ERROR(Event, "Error initializing event manager: fail to setup signal handler");
    }

    LOG_ADVANCED(Event, "Event manager initialized");

    // Init the semaphore
    sem_init(&event_semaphore, 0, 0);

    // Now create event thread
    pthread_t   event_thread;
    pthread_create(&event_thread, NULL, event_manager_handle_event, NULL);
    pthread_detach(event_thread);
}

void event_received(int sig)
{
    sem_post(&event_semaphore);
}

/**
 * Events can be of following types: 
 * DT_SENSOR - Event from sensor, has data of type 
 * DT_SENSOR_EVENT_DATA 
 *  
 * DT_XXXX - Event from service XXXX, has data of type 
 * DT_SERVICE_EVENT_DATA 
 * 
 * @author alex (3/11/2016)
 * 
 * @param arg 
 * 
 * @return void* 
 */
void* event_manager_handle_event(void* arg)
{
    while(true)
    {
        sem_wait(&event_semaphore);
        //pthread_mutex_lock(&event_received_cond_mutex);
        //pthread_cond_wait(&event_received_condition, &event_received_cond_mutex);
        LOG_DEBUG(Event, "Event received");
        event_t* event = event_receive();
        //pthread_mutex_unlock(&event_received_cond_mutex);

        scene_manager_on_event(event);
        service_manager_on_event(event);
        event_delete(event);
    }
}
