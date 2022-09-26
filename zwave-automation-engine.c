#include <stdio.h>
#include <ZWayLib.h>
#include <ZLogging.h>
#include "device_callbacks.h"
#include "resolver.h"
#include "command_parser.h"
#include "scene_manager.h"
#include "service_manager.h"
//#include "event_manager.h"
#include "logger.h"
#include "cli_commands.h"
#include "cli_auth.h"
#include "vty_io.h"
#include <signal.h>
#include <setjmp.h>
//#include <readline/readline.h>
//#include <readline/history.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "config.h"
#include "logging_modules.h"
#include <sys/types.h>
#include <dirent.h>
#include "user_manager.h"
#include "http_server.h"
#include "builtin_service_manager.h"
#include <event.h>
#include "vdev_manager.h"
#include <execinfo.h>
#include "cli_rest.h"
#include <termios.h>
#include "event_log.h"
#include "sensor_manager.h"
#include "homebridge_manager.h"
#include "event_dispatcher.h"
#include "python_manager.h"
#include <pwd.h>
#include <stdbool.h>
#include <socket.h>

#define DEFAULT_PORT 9231

ZWay zway;

DECLARE_LOGGER(General)
vty_io_data_t eventlog_vty_data;

/* For sigsetjmp() & siglongjmp(). */
sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call. */
static int jmpflag = 0;

//jmp_buf  exit_jmpbuf;
int keep_running = 1;

/* SIGTSTP handler.  This function care user's ^Z input. */
void sigtstp (int sig)
{
  /* Execute "end" command. */
  cli_command_exec_default("end");
  
  /* Initialize readline. */
  //rl_initialize ();
  printf ("\n");
 
  /* Check jmpflag for duplicate siglongjmp(). */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* SIGINT handler.  This function care user's ^C input.  */
void
sigint (int sig)
{
  //rl_initialize ();
  printf ("\n");
  //rl_forced_update_display ();
}

void sigpipe(int sig)
{
    keep_running = 0;
    if (! jmpflag)
    return;

    jmpflag = 0;
    //rl_done = 1;
    //siglongjmp (jmpbuf, 1);
}

void sigsegv(int sig) {
  void *array[100];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 100);

  // print out all the frames to stderr
  FILE* btfile = fopen("/tmp/zaebt", "w+");
  fprintf(btfile, "Error: signal %d, btsize %d\n", sig, size);
  backtrace_symbols_fd(array, size, fileno(btfile));

  fclose(btfile);
  exit(1);
}

void sighup(int sig)
{
    homebridge_manager_stop();
    //event_manager_shutdown();
    zway_stop(zway);
    zway_terminate(&zway);
    exit(1);
}

/* Signale wrapper for vtysh. We don't use sigevent because
 * vtysh doesn't use threads. TODO */
void
main_signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);
}

/* Initialization of signal handles. */
void signal_init()
{
  //main_signal_set (SIGTSTP, sigtstp);
  main_signal_set(SIGPIPE, sigpipe);
  //main_signal_set(SIGSEGV, sigsegv);
  main_signal_set(SIGHUP, sighup);
}

void setup_event_log(const char *filename)
{
    char event_log_path[512] = {0};
    snprintf(event_log_path, 511, "%s/%s", global_config.config_location, filename);

    eventlog_vty_data.desc.file = fopen(event_log_path, "w");

    setbuf(eventlog_vty_data.desc.file, NULL);

    vty_t* file_vty = vty_io_create(VTY_FILE, &eventlog_vty_data);
    logger_register_target(file_vty);
    logger_set_online(file_vty, true);
}

void print_help()
{
    printf("-c <config_file>\n-u <user>\n-d daemonize\n-l <filename> Write event log\n");
}

typedef struct stdin_event_context_t
{
    vty_t* vty;
    struct termios org_opts;

} stdin_event_context_t;

void on_stdin_event(event_pump_t* pump, int fd, void* context)
{
    stdin_event_context_t* c = (stdin_event_context_t*)context;
    vty_t* vty_std = c->vty;
    char* str = vty_read(vty_std);

    if(vty_is_command_received(vty_std))
    {
        // event_post ... COMMAND_RECEIVED
        vty_new_line(vty_std);
        cli_command_exec(vty_std, str);
        vty_display_prompt(vty_std);
        vty_flush(vty_std);
    }
    else if(vty_is_error(vty_std))
    {
        LOG_ERROR(General, "Input error");
    }

    if(vty_is_shutdown(vty_std))
    {
        LOG_ADVANCED(General, "Goodbye");
        logger_unregister_target(vty_std);
        vty_free(vty_std);
        tcsetattr(STDIN_FILENO, TCSANOW, &c->org_opts);
        homebridge_manager_stop();
        //event_manager_shutdown();
        zway_stop(zway);
        zway_terminate(&zway);
        event_dispatcher_stop();
    }
}

int main (int argc, char *argv[])
{
    bool is_daemon = false;
    char* config_file = NULL;
    char* setuid_name = NULL;
    char* eventlog_name = NULL;
    int c;

    while((c = getopt(argc, argv, "dhc:u:l:")) != -1)
    {
        switch(c)
        {
        case 'd':
            is_daemon = true;
            break;
        case 'c':
            config_file = optarg;
            break;
        case 'u':
            setuid_name = optarg;
            break;
        case 'l':
            eventlog_name = optarg;
            break;
        case 'h':
            print_help();
            exit(1);
            break;
        }
    }

    if(NULL != setuid_name)
    {
        struct passwd* pw = getpwnam(setuid_name);
        if(NULL == pw || setuid(pw->pw_uid) != 0)
        {
            printf("Error setting UID of %s\n", setuid_name);
            exit(1);
        }
    }

    pid_t pid = -1;
    if(is_daemon)
    {
        pid = fork();
    }
    else
    {
        main_signal_set (SIGINT, sigint);
        main_signal_set (SIGTSTP, sigtstp);
    }

    if(pid > 0)
    {
        exit(0);
    }
    else if(pid == 0)
    {
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
    }

    variant_init_pool();
    event_pool_init();
    logger_init();
    logging_modules_init();

    LOG_INFO(General, "Initializing engine...");

    if(!config_load(config_file))
    {
        printf("Error loading config file\n");
        return 1;
    }

    // Make sure config location exists
    DIR* dir = opendir(global_config.config_location);
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        /* Directory does not exist. */
        mkdir(global_config.config_location, 0755);
    }

    memset(&zway, 0, sizeof(zway));
    ZWLog logger = zlog_create(stdout, global_config.api_debug_level);

    char   zway_config[512] = {0};
    char   zway_translations[512] = {0};
    char   zway_zddx[512] = {0};

    snprintf(zway_config, 511, "%s/config", global_config.api_prefix);
    snprintf(zway_translations, 511, "%s/translations", global_config.api_prefix);
    snprintf(zway_zddx, 511, "%s/ZDDX", global_config.api_prefix);

    ZWError r = zway_init(&zway, global_config.device, zway_config,
                zway_translations,
                zway_zddx, "ZAE", logger);

    if(r == NoError)
    {
        LOG_INFO(General, "Zway API initialized");
        event_dispatcher_init();
        event_log_init();
        resolver_init();
        cli_init();
        service_manager_init(global_config.services_prefix);
        vdev_manager_init(global_config.vdev_prefix);
        scene_manager_init();
        user_manager_init();
        builtin_service_manager_init();
        sensor_manager_init();

        cli_load_config();
        
        vdev_manager_start_devices();
        python_manager_init(global_config.python_prefix);

        event_pump_t* pump = event_dispatcher_get_pump("EVENT_PUMP");
        zway_device_add_callback(zway, DeviceAdded, device_added_callback, (void*)pump);
        zway_device_add_callback(zway, CommandAdded, command_added_callback, (void*)pump);
        zway_device_add_callback(zway, DeviceRemoved, command_removed_callback, (void*)pump);

        LOG_INFO(General, "Engine data initialized");

        r = zway_start(zway, NULL, NULL);
    
        if(r == NoError)
        {
            // Ok, zway is started, now lets do local initialization

            if(NULL != eventlog_name)
            {
                setup_event_log(eventlog_name);
            }

            zway_discover(zway);

            LOG_INFO(General, "Engine ready");
           
            signal_init();

            socket_create_server(global_config.client_port, &cli_commands_handle_connect_event, NULL);
            http_server_create(8088, &cli_rest_handle_data_read_event);
            http_server_create(8089, &cli_commands_handle_http_data_event);

            homebridge_manager_init();
            //event_register_fd(homebridge_fd, &homebridge_on_event, NULL);

            // Create STD vty
            vty_t* vty_std = NULL;
            stdin_event_context_t context;
            
            if(!is_daemon)
            {
                struct termios org_opts, new_opts;
                int res=0;
                res=tcgetattr(STDIN_FILENO, &org_opts);
                //assert(res==0);
                    //---- set new terminal parms --------
                memcpy(&new_opts, &org_opts, sizeof(new_opts));
                new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL | ECHOCTL);
                tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
                context.org_opts = org_opts;

                vty_io_data_t* vty_data = calloc(1, sizeof(vty_io_data_t));
                vty_data->desc.io_pair[0] = stdin;
                vty_data->desc.io_pair[1] = stdout;
                vty_std = vty_io_create(VTY_STD, vty_data);
                logger_register_target(vty_std);
                cli_set_vty(vty_std);
                vty_display_banner(vty_std);
                        
                if(user_manager_get_count() > 0 && !vty_is_authenticated(vty_std))
                {
                    cmd_enter_auth_node(vty_std);
                }

                vty_display_prompt(vty_std);
                context.vty = vty_std;

                event_dispatcher_register_handler(event_dispatcher_get_pump("SOCKET_PUMP"), fileno(stdin), on_stdin_event, NULL, (void*)&context);
            }

            event_dispatcher_start();
        }
        else
        {
            LOG_ERROR(General, "Error starting Zway: %d", r);
            zway_log_error(zway, Critical, "Failed to start ZWay", r);
        }
    }
    else
    {
        LOG_ERROR(General, "Error initializing Zway: %d", r);
    }

    return(0);
}


