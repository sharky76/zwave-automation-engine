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
  //FILE* btfile = fopen("/tmp/zaebt", "w+");
  printf("Error: signal %d, btsize %d:\n", sig, size);
  backtrace_symbols_fd(array, size, 1);
  //fclose(btfile);
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
  main_signal_set (SIGPIPE, sigpipe);
  //signal(SIGSEGV, sigsegv);
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
        event_manager_init();
        resolver_init();
        cli_init();

        service_manager_init(global_config.services_prefix);
        vdev_manager_init(global_config.vdev_prefix);
        scene_manager_init();
        user_manager_init();
        builtin_service_manager_init();

        cli_load_config();
        
        vdev_manager_start_devices();
        zway_device_add_callback(zway, DeviceAdded, device_added_callback, NULL);
        zway_device_add_callback(zway, CommandAdded, command_added_callback, NULL);
        zway_device_add_callback(zway, DeviceRemoved, command_removed_callback, NULL);

        LOG_INFO(General, "Engine data initialized");

        r = zway_start(zway, NULL, NULL);
    
        if(r == NoError)
        {
            // Ok, zway is started, now lets do local initialization

            // Then, load automation scripts descriptor files
            

            zway_discover(zway);

            LOG_INFO(General, "Engine ready");
            
            //rl_editing_mode = 1;
            //rl_attempted_completion_function = &cli_command_completer;
            //rl_completion_entry_function = &null_function;
            //rl_bind_key ('?', (rl_command_func_t *) cli_command_describe);
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

            variant_stack_t* client_socket_list = stack_create();
            
            // Create STD vty
            vty_t* vty_std = NULL;
            struct termios org_opts, new_opts;

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

                vty_data_t* vty_data = calloc(1, sizeof(vty_data_t));
                vty_data->desc.io_pair[0] = stdin;
                vty_data->desc.io_pair[1] = stdout;
                vty_std = vty_io_create(VTY_STD, vty_data);

                cli_set_vty(vty_std);
                vty_display_banner(vty_std);
                        
                if(user_manager_get_count() > 0 && !vty_is_authenticated(vty_std))
                {
                    cmd_enter_auth_node(vty_std);
                }

                vty_display_prompt(vty_std);
            }

            while(true)
            {
                FD_ZERO (&fds);
                FD_SET(cli_sock,    &fds);    
                FD_SET(http_socket, &fds);
                
                stack_for_each(client_socket_list, vty_variant)
                {
                    vty_t* vty_ptr = VARIANT_GET_PTR(vty_t, vty_variant);
                    FD_SET(vty_ptr->data->desc.socket, &fds);
                }

                if(!is_daemon)
                {
                    FD_SET(STDIN_FILENO, &fds);
                }

                r = select (FD_SETSIZE, &fds, NULL, NULL, NULL);

                if(r <= 0)
                {
                    continue;
                }

                if(!is_daemon)
                {
                    if(FD_ISSET(STDIN_FILENO, &fds))
                    {
                        stdout_logger_data_t log_data = { STDOUT_FILENO };
                        logger_set_data(&log_data);
                
                        char* str = vty_read(vty_std);

                        if(vty_is_command_received(vty_std))
                        {
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
                            vty_free(vty_std);
                            tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
                            zway_stop(zway);
                            zway_terminate(&zway);

                            stack_for_each(client_socket_list, vty_variant)
                            {
                                vty_t* vty_ptr = VARIANT_GET_PTR(vty_t, vty_variant);
                                vty_free(vty_ptr);
                            }

                            exit(0);
                        }
                    }
                }
                if(FD_ISSET(cli_sock, &fds))
                {
                    session_sock = accept(cli_sock, NULL, NULL);
                        
                    stdout_logger_data_t log_data = { session_sock };
                    logger_set_data(&log_data);
    
                    LOG_ADVANCED(General, "Remote client connected");
    
                    vty_data_t* vty_data = calloc(1, sizeof(vty_data_t));
                    vty_data->desc.socket = session_sock;
                    vty_sock = vty_io_create(VTY_SOCKET, vty_data); 

                    stack_push_back(client_socket_list, variant_create_ptr(DT_PTR, vty_sock, NULL));

                    cli_set_vty(vty_sock);

                    vty_display_banner(vty_sock);
                        
                    if(user_manager_get_count() > 0)
                    {
                        cmd_enter_auth_node(vty_sock);
                    }
                
                    vty_display_prompt(vty_sock);
                }
                else if(FD_ISSET(http_socket, &fds))
                {
                    session_sock = accept(http_socket, NULL, NULL);
                    vty_data_t* vty_data = malloc(sizeof(vty_data_t));
                    vty_data->desc.socket = session_sock;

                    vty_sock = vty_io_create(VTY_HTTP, vty_data);
                    vty_set_echo(vty_sock, false);
                    char* str = vty_read(vty_sock);
                    cli_command_exec_custom_node(rest_root_node, vty_sock, str);
                    vty_free(vty_sock);
                }
                else
                {
                    stack_for_each(client_socket_list, vty_variant)
                    {
                        vty_t* vty_ptr = VARIANT_GET_PTR(vty_t, vty_variant);
                        if(FD_ISSET(vty_ptr->data->desc.socket, &fds))
                        {
                            char* str = vty_read(vty_ptr);

                            if(vty_is_command_received(vty_ptr))
                            {
                                vty_new_line(vty_ptr);
                                cli_command_exec(vty_ptr, str);
                                vty_display_prompt(vty_ptr);
                                vty_flush(vty_ptr);
                            }
                            else if(vty_is_error(vty_ptr))
                            {
                                LOG_ERROR(General, "Socket error");
                                vty_free(vty_ptr);
                                stack_remove(client_socket_list, vty_variant);
                            }

                            if(vty_is_shutdown(vty_ptr))
                            {
                                LOG_ADVANCED(General, "Remote client disconnected");
                                vty_free(vty_ptr);
                                stack_remove(client_socket_list, vty_variant);
                            }
                        }
                    }
                }
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

