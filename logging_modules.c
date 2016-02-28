#include "logging_modules.h"
#include <logger.h>

void logging_modules_init()
{
    REGISTER_LOGGER(General)
    REGISTER_LOGGER(Parser)
    REGISTER_LOGGER(SceneGeneral)
    REGISTER_LOGGER(SceneAction)
    REGISTER_LOGGER(SceneManager)
    REGISTER_LOGGER(Event)
    REGISTER_LOGGER(Config)
    REGISTER_LOGGER(DataCallback)
    REGISTER_LOGGER(DeviceCallback)
    REGISTER_LOGGER(Resolver)
    REGISTER_LOGGER(ServiceManager)
    REGISTER_LOGGER(CLI_SCENE)
}
