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
    REGISTER_LOGGER(User)
    REGISTER_LOGGER(HTTPServer)
    REGISTER_LOGGER(BuiltinService)
    REGISTER_LOGGER(VDevManager)
    REGISTER_LOGGER(SocketIO)
    REGISTER_LOGGER(EventLog)
    REGISTER_LOGGER(HomebridgeManager)
    REGISTER_LOGGER(CLI)
    REGISTER_LOGGER(EventPump)
    REGISTER_LOGGER(PythonManager)
}
