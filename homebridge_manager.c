#include "homebridge_manager.h"
#include "config.h"
#include "logger.h"
#include "event.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <event_dispatcher.h>

DECLARE_LOGGER(HomebridgeManager)

int homebridge_start();
void homebridge_stopped_handler(int sig, siginfo_t *info, void *arg);

pid_t homebridge_pid = -1;
int homebridge_fd = -1;
static bool can_start_homebridge = false;
static bool homebridge_restart_on_crash = false;
void homebridge_on_event(event_pump_t *pump, int homebridge_fd, void *context);

void homebridge_manager_set_start_state(bool isStart)
{
    can_start_homebridge = isStart;
}

int homebridge_manager_init()
{
    if (can_start_homebridge)
    {
        LOG_ADVANCED(HomebridgeManager, "Initializing Homebridge manager...");
        homebridge_restart_on_crash = true;
        if (-1 == homebridge_fd)
        {
            homebridge_fd = homebridge_start();
            event_pump_t *pump = event_dispatcher_get_pump("SOCKET_PUMP");
            event_dispatcher_register_handler(pump, homebridge_fd, &homebridge_on_event, NULL, NULL);
        }
    }
    return homebridge_fd;
}

int homebridge_start()
{
    int pipefd[2];
    if (0 != pipe2(pipefd, O_NONBLOCK))
    {
        LOG_ERROR(HomebridgeManager, "Failed to create process pipe");
        return -1;
    }

    homebridge_pid = fork();

    if (0 != homebridge_pid)
    {
        // parent
        struct sigaction sa;
        sa.sa_sigaction = &homebridge_stopped_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP;
        if (sigaction(SIGCHLD, &sa, 0) == -1)
        {
            perror(0);
            exit(1);
        }

        // Close write part of the pipe
        LOG_DEBUG(HomebridgeManager, "Homebridge read fd: %d", pipefd[0]);
        close(pipefd[1]);
        return pipefd[0];
    }
    else
    {
        char *args[] = {NULL, NULL, NULL, NULL};
        args[0] = "homebridge";
        args[1] = "-P";
        args[2] = global_config.homebridge_plugin_path;

        /* Create a new SID for the child process */
        pid_t sid = setsid();
        if (sid < 0)
        {
            exit(EXIT_FAILURE);
        }

        // Close read part of the pipe
        close(pipefd[0]);

        // Duplicate stdout and stderr to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        //close(STDOUT_FILENO);
        //close(STDERR_FILENO);

        /*resettign File Creation Mask */
        umask(027);

        execvp("homebridge", args);
        perror("execvp");
        exit(1);
    }
}

void homebridge_stopped_handler(int sig, siginfo_t *info, void *arg)
{
    if (info->si_pid != homebridge_pid)
    {
        return;
    }

    int saved_errno = errno;
    int status = 0;
    pid_t pid = waitpid(homebridge_pid, &status, WNOHANG);

    if (WIFEXITED(status))
    {
        event_pump_t *pump = event_dispatcher_get_pump("SOCKET_PUMP");
        event_dispatcher_unregister_handler(pump, homebridge_fd, NULL, NULL);
        if (homebridge_restart_on_crash)
        {
            homebridge_manager_init();
        }
    }

    errno = saved_errno;
}

void homebridge_manager_stop()
{
    homebridge_restart_on_crash = false;
    event_pump_t *pump = event_dispatcher_get_pump("SOCKET_PUMP");
    event_dispatcher_unregister_handler(pump, homebridge_fd, NULL, NULL);
    
    if(homebridge_pid != -1)
    {
        kill(homebridge_pid, SIGINT);
    }
    homebridge_fd = -1;
}

bool homebridge_manager_is_running()
{
    return homebridge_fd != -1;
}

void homebridge_on_event(event_pump_t *pump, int homebridge_fd, void *context)
{
    char buf[512] = {0};
    int nread = 0;

    nread = read(homebridge_fd, buf, 511);

    if(nread < 0)
    {
        event_pump_t *pump = event_dispatcher_get_pump("SOCKET_PUMP");
        event_dispatcher_unregister_handler(pump, homebridge_fd, NULL, NULL);
    }
    // Remove '\r\n'...
    if (nread > 1)
    {
        buf[nread - 1] = 0;
        LOG_INFO(HomebridgeManager, buf);
    }

}
