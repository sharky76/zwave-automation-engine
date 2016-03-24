#include <stdio.h>
#include <ZWayLib.h>
#include <ZLogging.h>
#include "device_callbacks.h"
#include "resolver.h"
#include "command_parser.h"
#include "scene_manager.h"
#include "service_manager.h"
#include "event_manager.h"
#include "logger.h"
#include "cli_commands.h"
#include "cli_auth.h"
#include "vty_io.h"
#include <signal.h>
#include <setjmp.h>
#include <readline/readline.h>
#include <readline/history.h>
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

#define DEFAULT_PORT 9231

ZWay zway;

DECLARE_LOGGER(General)

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
  //cli_command_exec_default("end");
  
  /* Initialize readline. */
  rl_initialize ();
  printf ("\n");
 
  /* Check jmpflag for duplicate siglongjmp(). */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* SIGINT handler.  This function care user's ^Z input.  */
void
sigint (int sig)
{
  rl_initialize ();
  printf ("\n");
  rl_forced_update_display ();
}

void sigpipe(int sig)
{
    keep_running = 0;
    if (! jmpflag)
    return;

    jmpflag = 0;
    rl_done = 1;
    //siglongjmp (jmpbuf, 1);
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
  //main_signal_set (SIGINT, sigint);
  //main_signal_set (SIGTSTP, sigtstp);
  main_signal_set (SIGPIPE, sigpipe);
}

char* null_function(const char *ignore, int key)
{
    return NULL;
}

void print_help()
{
    printf("-c <config_file>\n-d daemonize\n");
}

int main (int argc, char *argv[])
{
    bool is_daemon = false;
    char* config_file = NULL;
    int c;

    while((c = getopt(argc, argv, "dhc:")) != -1)
    {
        switch(c)
        {
        case 'd':
            is_daemon = true;
            break;
        case 'c':
            config_file = optarg;
            break;
        case 'h':
            print_help();
            exit(1);
            break;
        }
    }

    pid_t pid = -1;
    if(is_daemon)
    {
        pid = fork();
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

    stdout_logger_data_t log_data = { STDOUT_FILENO };
    logger_init(LOG_LEVEL_BASIC, LOG_TARGET_STDOUT, &log_data);
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
    ZWLog logger = zlog_create(stdout, 4);

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
        LOG_ADVANCED(General, "Zway API initialized");
        resolver_init();
        cli_init();

        service_manager_init(global_config.services_prefix);
        scene_manager_init();
        event_manager_init();
        user_manager_init();
        builtin_service_manager_init();

        cli_load_config();
        
        zway_device_add_callback(zway, DeviceAdded, device_added_callback, NULL);
        zway_device_add_callback(zway, CommandAdded, command_added_callback, NULL);
        zway_device_add_callback(zway, DeviceRemoved, command_removed_callback, NULL);

        LOG_ADVANCED(General, "Engine data initialized");

        r = zway_start(zway, NULL, NULL);
    
        if(r == NoError)
        {
            // Ok, zway is started, now lets do local initialization

            // Then, load automation scripts descriptor files
            

            zway_discover(zway);

            LOG_INFO(General, "Engine ready");
            
            rl_editing_mode = 1;
            rl_attempted_completion_function = &cli_command_completer;
            rl_completion_entry_function = &null_function;
            rl_bind_key ('?', (rl_command_func_t *) cli_command_describe);
            //rl_pre_input_hook = &cli_print_prompt_function;
            signal_init();
            
            /*vty_data_t data = {
                .desc.io_pair[0] = stdin,
                .desc.io_pair[1] = stdout
            };*/
            //vty_t* vty_console = vty_create(VTY_STD, &data);
            //cli_set_vty(vty_console);
            
            /* Preparation for longjmp() in sigtstp(). */
            
            

            /*while(keep_running)
            {
                char* str = vty_read(vty_console);
                cli_command_exec(vty_console, str);
                free(str);
            }*/
            


            int cli_sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(struct sockaddr_in));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(global_config.client_port);
            addr.sin_addr.s_addr = /*inet_addr("192.168.1.91");*/INADDR_ANY;
            if(-1 == bind(cli_sock, &addr, sizeof(struct sockaddr_in)))
            {
                perror("bind");
                return EXIT_FAILURE;
            }


            int on = 1;
            setsockopt(cli_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
            //setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int));

            listen(cli_sock, 1);
            int http_socket = http_server_get_socket(8088);

            fd_set fds;
            int session_sock = -1;
            vty_t* vty_sock;

            while(true)
            {
                FD_ZERO (&fds);
                FD_SET(cli_sock,    &fds);    
                FD_SET(http_socket, &fds);

                r = select (FD_SETSIZE, &fds, NULL, NULL, NULL);

                if(r <= 0)
                {
                    continue;
                }

                if(FD_ISSET(cli_sock, &fds))
                {
                    session_sock = accept(cli_sock, NULL, NULL);
                        
                    stdout_logger_data_t log_data = { session_sock };
                    logger_set_data(&log_data);
    
                    LOG_ADVANCED(General, "Remote client connected");
    
                    int saved_stdout = dup(STDOUT_FILENO);
                    int saved_stdin = dup(STDIN_FILENO);
    
                    close(STDOUT_FILENO);
                    dup2(session_sock, STDOUT_FILENO);
                    close(STDIN_FILENO);
                    dup2(session_sock, STDIN_FILENO);
        
                    //FILE* readline_io = fdopen(session_sock, "rw");
                    vty_data_t vty_data = {
                        .desc.io_pair[0] = stdin,
                        .desc.io_pair[1] = stdout
                    };
    
                    vty_sock = vty_io_create(VTY_STD, &vty_data);
                    cli_set_vty(vty_sock);
                    vty_display_banner(vty_sock);

                    if(user_manager_get_count() > 0)
                    {
                        cmd_enter_auth_node(vty_sock);
                    }
                
                    //sigsetjmp(exit_jmpbuf, 1);
                    sigsetjmp (jmpbuf, 1);
                    jmpflag = 1;
    
                    while(keep_running)
                    {
                        char* str = vty_read(vty_sock);
                        cli_command_exec(vty_sock, str);
                        rl_on_new_line();
                        free(str);
                    }
    
                    keep_running = 1;
                    LOG_ADVANCED(General, "Remote client disconnected");
                } 
                else if(FD_ISSET(http_socket, &fds))
                {
                    session_sock = accept(http_socket, NULL, NULL);
                    vty_data_t vty_data = {
                        .desc.socket = session_sock
                    };

                    vty_sock = vty_io_create(VTY_HTTP, &vty_data);
                    char* str = vty_read(vty_sock);
                    cli_command_exec(vty_sock, str);
                }

                vty_free(vty_sock);
                close(session_sock);
                
                //dup2(saved_stdout, 1);
                //dup2(saved_stdin, 0);

                
            }












            //rl_ding();
            /*rl_callback_handler_install ("# ", cli_command_exec_default);

            // Create TCP socket for listening
            int cli_sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(struct sockaddr_in));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(3333);
            addr.sin_addr.s_addr = INADDR_ANY;
            bind(cli_sock, &addr, sizeof(struct sockaddr_in));
            listen(cli_sock, 1);

            int client_sock = -1;
            fd_set fds;
            while (keep_running)
            {
              FD_ZERO (&fds);
              FD_SET(fileno (rl_instream), &fds);    
              FD_SET(cli_sock, &fds);

              r = select (cli_sock+1, &fds, NULL, NULL, NULL);
              if (r < 0)
                {
                  //perror ("rltest: select");
                  //rl_callback_handler_remove ();
                  //break;
                }
        
              else if (FD_ISSET (fileno (rl_instream), &fds))
              {
                    rl_callback_read_char ();
              }
              else if(FD_ISSET(cli_sock, &fds))
              {
                  client_sock = accept(cli_sock, NULL, NULL);
                  close(STDOUT_FILENO);
                  dup2(client_sock, STDOUT_FILENO);
                  close(STDIN_FILENO);
                  dup2(client_sock, STDIN_FILENO);
                  close(STDERR_FILENO);
                  dup2(client_sock, STDERR_FILENO);
              }
            }*/

            //vty_free(vty_console);
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

