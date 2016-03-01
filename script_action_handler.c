#include "script_action_handler.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <logger.h>
#include <fcntl.h>

USING_LOGGER(Scene);

typedef struct env_table_t
{
    pid_t   pid;
    char**  envp;
} env_table_t;

env_table_t stored_env_array[MAX_SIMULTANEOUS_SCRIPTS] = {0};

void store_env(pid_t pid, char** envp)
{
    // Find empty space
    for(int i = 0; i < sizeof(stored_env_array) / sizeof(env_table_t); i++)
    {
        if(stored_env_array[i].pid == 0)
        {
            stored_env_array[i].pid = pid;
            stored_env_array[i].envp = envp;
            return;
        }
    }
}

void clear_env(pid_t pid)
{
    // Find the saved pid
    for(int i = 0; i < sizeof(stored_env_array) / sizeof(env_table_t); i++)
    {
        if(stored_env_array[i].pid == pid)
        {
            char** env_to_delete = stored_env_array[i].envp;

            char* item = *env_to_delete;
            while(NULL != item)
            {
                free(item);
                env_to_delete++;
                item = *env_to_delete;
            }

            free(stored_env_array[i].envp);

            stored_env_array[i].envp = NULL;
            stored_env_array[i].pid = 0;
        }
    }
}

void script_done_handler(int sig)
{
  int saved_errno = errno;
  pid_t pid = waitpid((pid_t)(-1), 0, 0);
  //while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  LOG_DEBUG(Scene, "Script done!");
  clear_env(pid);
  errno = saved_errno;
}

void script_exec(const char* path, char** envp)
{
    pid_t pid = fork();

    if(0 != pid)
    {
        store_env(pid, envp);

        // parent
        struct sigaction sa;
        sa.sa_handler = &script_done_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
        if (sigaction(SIGCHLD, &sa, 0) == -1) 
        {
            perror(0);
            exit(1);
        }
    }
    else
    {
        char* args[] = { NULL, NULL };
        args[0] = (char*)path;

        LOG_DEBUG(Scene, "Executing %s", path);

        /* Create a new SID for the child process */  
        pid_t sid = setsid();  
        if (sid < 0)    
        {  
            exit(EXIT_FAILURE);  
        }  
    
        int fd = open("/dev/null",O_RDWR, 0);  
    
        if (fd != -1)  
        {  
            dup2 (fd, STDIN_FILENO);  
            dup2 (fd, STDOUT_FILENO);  
            dup2 (fd, STDERR_FILENO);  
      
            if (fd > 2)  
            {  
                close (fd);  
            }  
        }  
      
        /*resettign File Creation Mask */  
        umask(027); 

        execve(path, args, envp);
        perror("execle");
        exit(1);
    }
}

