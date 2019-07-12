# Z-WAVE Home Automation Engine
Create and manage home automation scenes which are activated by certain criteria

Uses [Z-Way C API](https://z-wave.me/z-way/) to communicate with the Razberry HW.
On command-class data change can trigger a pre-defined scenes.
Scene contains multiple actions including calling external script with custom environment,
calling different scene, or run a service command.
Service is an external plugin which provides extended commands to the scene actions. Look in services/ for some examples
Provides easy to use CLI for creation of scenes and managing Z-WAVE network and devices

Look at Makefile to find out how to build this. 
Requires JSON-C and some other more well known libraries
