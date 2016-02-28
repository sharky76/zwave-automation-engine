#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>

#define DEFAULT_PORT 9231

/* SIGTSTP handler.  This function care user's ^Z input. */
int got_sigtstp = 0;
int got_sigint = 0;
void sigtstp (int sig)
{
  got_sigtstp = 1;
}

/* SIGINT handler.  This function care user's ^Z input.  */
void
sigint (int sig)
{
    got_sigint = 1;
}

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
  //main_signal_set (SIGPIPE, SIG_IGN);
}

int main (int argc, char *argv[])
{
    struct termios org_opts, new_opts;
    int res=0;
        //-----  store old settings -----------
    res=tcgetattr(STDIN_FILENO, &org_opts);
    //assert(res==0);
        //---- set new terminal parms --------
    memcpy(&new_opts, &org_opts, sizeof(new_opts));
    new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL | ECHOCTL);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);

    signal_init();

    int cli_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);
    addr.sin_addr.s_addr = inet_addr("192.168.1.91");
    int on = 1;
    setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int));

    fd_set read_fds;
    fd_set except_fds;

    if(0 == connect(cli_sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)))
    {
        int flags = fcntl(cli_sock, F_GETFL);
        //fcntl(cli_sock, F_SETFL, flags | O_NONBLOCK);

        //char cr = '\n';
        //write(cli_sock, &cr, sizeof(cr));
        char buf[1024] = {0};
        //int nread = read(cli_sock, buf, sizeof(buf));
        //printf("%s", buf);

        sigset_t    blockset;
        sigset_t    emptyset;
        sigemptyset(&blockset);
        sigemptyset(&emptyset);
        sigaddset(&blockset, SIGTSTP);
        sigaddset(&blockset, SIGINT);
        sigprocmask(SIG_BLOCK, &blockset, NULL);

        bool keep_running = true;
        char escape_seq_buf[3];

        while(keep_running)
        {
            FD_ZERO(&read_fds);
            FD_ZERO(&except_fds);
            FD_SET(0, &read_fds);
            FD_SET(cli_sock, &read_fds);
            FD_SET(cli_sock, &except_fds);
            //FD_ZERO(&write_fds)
            //FD_SET(1, &write_fds);
            //FD_SET(cli_sock, &write_fds);
            
            int ret = pselect(cli_sock+1, &read_fds, NULL, &except_fds, NULL, &emptyset);

            if(ret > 0)
            {
                if(FD_ISSET(0, &read_fds))
                {
                    char ch = getchar();
                    if (ch == 27) {
                        escape_seq_buf[0] = ch;
                        // get next two characters which specify escape sequence
                        escape_seq_buf[1] = getchar();
                        escape_seq_buf[2] = getchar();
                        write(cli_sock, escape_seq_buf, 3);
                    }
                    else
                    {
                        write(cli_sock, &ch, sizeof(ch));
                    }
                }

                if(FD_ISSET(cli_sock, &read_fds))
                {
                    int nread = sizeof(buf);
                    while(nread == sizeof(buf))
                    {
                        nread = recv(cli_sock, buf, sizeof(buf), 0);
    
                        if(nread <= 0)
                        {
                            keep_running = false;
                        }
                        int i = 0;
                        for(i = 0; i < sizeof(buf); i++)
                        {
                            if(buf[i] != 0)
                            {
                                fprintf(stdout, "%c", buf[i]);
                            }
                        }
    
                        fflush(stdout);
                        memset(buf, 0, sizeof(buf));
                    }
                }

                if(FD_ISSET(cli_sock, &except_fds))
                {
                    keep_running = false;
                }
            }
            else if(errno == EINTR)
            {
                if(got_sigtstp == 1)
                {
                    got_sigtstp = 0;
                    char endbuf[4] = {"end\n"};

                    write(cli_sock, endbuf, sizeof(endbuf));
                }
                else if(got_sigint == 1)
                {
                    got_sigint = 0;
                    char endbuf[5] = {"exit\n"};
                    write(cli_sock, endbuf, sizeof(endbuf));
                    keep_running = false;
                    printf("\n");
                }
            }
            else
            {
                // Stop the client!
                keep_running = false;
            }
        }

        close(cli_sock);
    }

    //------  restore old settings ---------
   res=tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
   //assert(res==0);

   return(0);
}

