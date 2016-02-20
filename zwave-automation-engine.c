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
#include "vty.h"
#include "data_cache.h"
#include <signal.h>
#include <setjmp.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

ZWay zway;

/* For sigsetjmp() & siglongjmp(). */
static sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call. */
static int jmpflag = 0;

jmp_buf  exit_jmpbuf;
int keep_running = 1;

/* SIGTSTP handler.  This function care user's ^Z input. */
void sigtstp (int sig)
{
  /* Execute "end" command. */
  cli_command_exec_default("end");
  
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
  main_signal_set (SIGINT, sigint);
  main_signal_set (SIGTSTP, sigtstp);
  main_signal_set (SIGPIPE, SIG_IGN);
}

char* null_function(const char *ignore, int key)
{
    return NULL;
}

int main (int argc, char *argv[])
{
    stdout_logger_data_t log_data = { stdout };
    logger_init(LOG_LEVEL_DEBUG, LOG_TARGET_STDOUT, &log_data);

    LOG_INFO("Initializing engine...");

    memset(&zway, 0, sizeof(zway));
    ZWLog logger = zlog_create(stdout, 4);
    ZWError r = zway_init(&zway, "/dev/ttyAMA0", "/home/osmc/z-way-server/config",
                "/home/osmc/z-way-server/translations",
                "/home/osmc/z-way-server/ZDDX", "ZAE", logger);

    

    if(r == NoError)
    {
        LOG_ADVANCED("Zway API initialized");
        cli_init();
        resolver_init();

        service_manager_init("services");
        scene_manager_init("scenes");
        event_manager_init();
        //data_cache_init();

        cli_load_config();
        
        zway_device_add_callback(zway, DeviceAdded, device_added_callback, NULL);
        zway_device_add_callback(zway, CommandAdded, command_added_callback, NULL);
        zway_device_add_callback(zway, DeviceRemoved, command_removed_callback, NULL);

        LOG_ADVANCED("Engine data initialized");

        r = zway_start(zway, NULL, NULL);
    
        if(r == NoError)
        {
            // Ok, zway is started, now lets do local initialization

            // Then, load automation scripts descriptor files
            

            zway_discover(zway);

            LOG_INFO("Engine ready");

            rl_editing_mode = 1;
            rl_attempted_completion_function = &cli_command_completer;
            rl_completion_entry_function = &null_function;
            rl_bind_key ('?', (rl_command_func_t *) cli_command_describe);

            signal_init();
            
            vty_data_t data = {
                .desc.io_pair[0] = stdin,
                .desc.io_pair[1] = stdout
            };
            vty_t* vty_console = vty_create(VTY_STD, &data);
            cli_set_vty(vty_console);
            
            /* Preparation for longjmp() in sigtstp(). */
            sigsetjmp (jmpbuf, 1);
            jmpflag = 1;
            setjmp(exit_jmpbuf);

            while(keep_running)
            {
                char* str = vty_read(vty_console);
                cli_command_exec(vty_console, str);
                free(str);
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

            vty_free(vty_console);
        }
        else
        {
            LOG_ERROR("Error starting Zway: %d", r);
            printf("zway error %d\n", r);
            zway_log_error(zway, Critical, "Failed to start ZWay", r);
        }
    }
    else
    {
        LOG_ERROR("Error initializing Zway: %d", r);
        printf("Error initializing zway %d!\n", r);
    }

    return(0);
}

