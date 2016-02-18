# zwave-automation-engine
Create and manage scenes which are activated by certain criteria

Uses Z-Way C API to communicate with the HW.
On command-class data change can trigger a pre-defined scene.
Scene contains multiple actions including calling external script with custom environment,
calling different scene, or run a service command.
Service is an external plugin which provides extended commands to the scene actions. Look in services/ for some examples
