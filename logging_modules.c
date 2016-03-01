#include "logging_modules.h"
#include <logger.h>

void logging_modules_init()
{
    REGISTER_LOGGER(General)
    REGISTER_LOGGER(Parser)
    REGISTER_LOGGER(Scene)
    REGISTER_LOGGER(Event)
    REGISTER_LOGGER(Config)
    REGISTER_LOGGER(DataCallback)
    REGISTER_LOGGER(DeviceCallback)
    REGISTER_LOGGER(Resolver)
    REGISTER_LOGGER(ServiceManager)
}
