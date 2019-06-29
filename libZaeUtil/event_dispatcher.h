#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

// Event pumps either push events into event manager
// or can be polled for events
// Eg: Socket event pump when polled executes "poll" call on socket descriptor and return socket descriptor which is ready for read or write
//     Timer event pump when polled increment internal counter and return event if counter reached pre-set value

typedef struct event_pump_t
{
    char* name;
    void (*poll)(event_pump_t*, time_t);
    void (*register_handler)(event_pump_t*, ...);
    void (*unregister_handler)(event_pump_t*, ...);
    void (*start)(event_pump_t*, ...);
    void (*stop)(event_pump_t*, ...);
    void* priv;
} event_pump_t;

void event_dispatcher_init();
void event_dispatcher_start();
void event_dispatcher_stop();

void event_dispatcher_register_handler(event_pump_t*, ...);
void event_dispatcher_unregister_handler(event_pump_t*, ...);

event_pump_t* event_dispatcher_get_pump(const char* name);
