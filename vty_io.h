#pragma once 
#include <vty.h>
/*
 
Define virtual TTY handle for CLI command module 
 
*/

vty_t*  vty_io_create(vty_type type, vty_data_t* data);
void    vty_io_config(vty_t* vty);

