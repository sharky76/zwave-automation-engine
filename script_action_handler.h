/*
Handler for script scene actions 
 
forks and execs the script, and reaps the dead child 
*/

#define MAX_SIMULTANEOUS_SCRIPTS    5

void script_exec(const char* path, char** envp);

