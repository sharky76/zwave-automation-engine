#include "EventLog.h"
#include "logger.h"
#include "event_log.h"

USING_LOGGER(BuiltinService)

variant_t*  add_impl(struct service_method_t* method, va_list args);
variant_t*  get_tail_impl(struct service_method_t* method, va_list args);

builtin_service_t*  event_log_service_create(hash_table_t* service_table)
{
    builtin_service_t*   service = builtin_service_create(service_table, "EventLog", "Event Log methods");
    builtin_service_add_method(service, "Add", "Add event log entry", 4, add_impl);
    builtin_service_add_method(service, "GetTail", "Return last X entries", 1, get_tail_impl);

    return service;
}

variant_t*  add_impl(struct service_method_t* method, va_list args)
{
    variant_t* node_id_variant = va_arg(args, variant_t*);
    variant_t* instance_id_variant = va_arg(args, variant_t*);
    variant_t* command_class_variant = va_arg(args, variant_t*);
    variant_t* event_string_variant = va_arg(args, variant_t*);

    event_entry_t* new_entry = new_event_entry();
    new_entry->node_id = variant_get_int(node_id_variant);
    new_entry->instance_id = variant_get_int(instance_id_variant);
    new_entry->command_id = variant_get_int(command_class_variant);

    variant_to_string(event_string_variant, &new_entry->event_data);

    event_send(new_entry, NULL);
    return variant_create_bool(true);
}

variant_t*  get_tail_impl(struct service_method_t* method, va_list args)
{
    variant_t* num_lines_variant = va_arg(args, variant_t*);
    return variant_create_list(event_log_get_tail(variant_get_int(num_lines_variant)));
}

