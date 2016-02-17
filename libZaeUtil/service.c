#include "service.h"
#include "event.h"
#include "variant.h"
#include <stdlib.h>

void    service_post_event(int source_id, const char* data)
{
    event_t* new_event = event_create(source_id, 
                                      variant_create_ptr(DT_SERVICE_EVENT_DATA, strdup(data), variant_delete_default));
    event_post(new_event);
}
