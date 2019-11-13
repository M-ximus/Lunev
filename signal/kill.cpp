#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

const char died_chld_msg[] = "Parent caught SIGCHLD\n";
const char died_parent_msg[] = "Child caugnt parent death\n";

const char zbit = '0';
const char hbit = '1';

int old_ppid = 0;

int Child_glob_check = 1;
volatile char cur_bit = 0;

static void died_child(int sig)
{
    //printf("%d\n", sig);
    write(2, died_chld_msg, 23);
    exit(EXIT_FAILURE);
}

static void chld_alarm(int sig)
{
    //printf("%d\n", sig);
    //static int old_ppid = getppid();
    int cur_ppid = getppid();
    //int new_ppid = getppid();
    //static ppid = getppid();
    if (old_ppid != cur_ppid)
    {
        write(2, died_parent_msg, 27);
        exit(EXIT_FAILURE);
    }
    Child_glob_check = 1;
}

static void handler_usr1(int sig)
{
    write(2, &zbit, 1);
    cur_bit = 0;
}

static void handler_usr2(int sig)
{
    write(2, &hbit, 1);
    cur_bit = 1;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Bad num of arguments\n");
        exit(EXIT_FAILURE);
    }

    old_ppid = getpid();

    errno = 0;
    sigset_t start_set;
    int ret = sigemptyset(&start_set);
    if (ret < 0)
    {
       perror("make empty start mask error");
       exit(EXIT_FAILURE);
    }
    ret = sigaddset(&start_set, SIGUSR1);
    if (ret < 0)
    {
       perror("add usr1 start mask error");
       exit(EXIT_FAILURE);
    }
    ret = sigaddset(&start_set, SIGUSR2);
    if (ret < 0)
    {
        perror("add usr2 start mask error");
        exit(EXIT_FAILURE);
    }
    ret = sigaddset(&start_set, SIGCHLD);
    if (ret < 0)
    {
        perror("add child start mask error");
        exit(EXIT_FAILURE);
    }

    sigset_t old_mask;
    errno = 0;
    ret = sigprocmask(SIG_BLOCK, &start_set, &old_mask);
    if (ret < 0)
    {
        perror("set start mask error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Bad fork");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        errno = 0;
        struct sigaction sa_child;
        sigemptyset(&sa_child.sa_mask);
        /*sigaddset(&sa_child.sa_mask, SIGUSR1);
        sigaddset(&sa_child.sa_mask, SIGUSR2);*/
        if (errno != 0)
        {
            perror("make parent sigchld empty mask error");
            exit(EXIT_FAILURE);
        }
        sa_child.sa_handler = died_child;
        sa_child.sa_flags = SA_NOCLDWAIT;

        errno = 0;
        ret = sigaction(SIGCHLD, &sa_child, NULL);
        if (ret < 0)
        {
            perror("Set handler sigchld error");
            exit(EXIT_FAILURE);
        }

        errno = 0;
        struct sigaction sa_usr1;
        ret = sigemptyset(&sa_usr1.sa_mask);
        if (ret < 0)
        {
            perror("make parent usr1 empty mask error");
            exit(EXIT_FAILURE);
        }
        sa_usr1.sa_handler = handler_usr1;

        errno = 0;
        ret = sigaction(SIGUSR1, &sa_usr1, NULL);
        if (ret < 0)
        {
            perror("Set handler usr1 error");
            exit(EXIT_FAILURE);
        }

        errno = 0;
        struct sigaction sa_usr2;
        ret = sigemptyset(&sa_usr2.sa_mask);
        if (ret < 0)
        {
            perror("make parent usr2 empty mask error");
            exit(EXIT_FAILURE);
        }
        sa_usr2.sa_handler = handler_usr2;
        sa_usr2.sa_flags = SA_NODEFER;

        errno = 0;
        ret = sigaction(SIGUSR2, &sa_usr2, NULL);
        if (ret < 0)
        {
            perror("Set handler usr2 error");
            exit(EXIT_FAILURE);
        }

        sigset_t run_mask;
        int ret = sigfillset(&run_mask);
        if (ret < 0)
        {
           perror("make fill start mask error");
           exit(EXIT_FAILURE);
        }
        ret = sigdelset(&run_mask, SIGUSR1);
        if (ret < 0)
        {
           perror("del usr1 start mask error");
           exit(EXIT_FAILURE);
        }
        ret = sigdelset(&run_mask, SIGUSR2);
        if (ret < 0)
        {
            perror("del usr2 start mask error");
            exit(EXIT_FAILURE);
        }
        ret = sigdelset(&run_mask, SIGCHLD);
        if (ret < 0)
        {
            perror("del child start mask error");
            exit(EXIT_FAILURE);
        }

        /*errno = 0;
        ret = sigprocmask(SIG_SETMASK, &old_mask, NULL);
        if (ret < 0)
        {
            perror("set old proc mask error");
            exit(EXIT_FAILURE);
        }*/

        while(1)
        {
            char cur_simb = 0;
            for(int i = 0; i < 8; ++i)
            {
                //printf("Curr parent bit %d\n", i);
                char mask = 0x01 << i;

                errno = 0;
                //printf("%d\n", run_mask);
                //fflush(0);
                sigsuspend(&run_mask);
                if (errno != EINTR)
                {
                    perror("Not interrupt error in parent suspend");
                    exit(EXIT_FAILURE);
                }

                if (cur_bit)
                    cur_simb = cur_simb | mask;
                else
                    cur_simb = cur_simb & (~mask);

                errno = 0;
                ret = kill(pid, SIGALRM);
                if (ret < 0)
                {
                    perror("Bad sigalarm to chld");
                    exit(EXIT_FAILURE);
                }
                //exit(0);
            }
            printf("%c", cur_simb);
        }

        return 0;
    }
    else
    {
        sigset_t mask_alarm;
        ret = sigemptyset(&mask_alarm);
        if (ret < 0)
        {
            perror("empty alarm mask error");
            exit(EXIT_FAILURE);
        }
        struct sigaction sa_chld_alarm;
        sa_chld_alarm.sa_mask = mask_alarm;
        sa_chld_alarm.sa_flags = 0;
        sa_chld_alarm.sa_handler = chld_alarm;

        ret = sigaction(SIGALRM, &sa_chld_alarm, NULL);

        sigset_t send_mask;
        ret = sigfillset(&send_mask);
        if (ret < 0)
        {
            perror("empty send mask error");
            exit(EXIT_FAILURE);
        }
        ret = sigdelset(&send_mask, SIGALRM);
        if (ret < 0)
        {
            perror("add ret sig error");
            exit(EXIT_FAILURE);
        }

        pid_t ppid = getppid();

        errno = 0;
        int fd = open(argv[1], O_RDONLY);
        if (fd < 0)
        {
            perror("Open file error");
            exit(EXIT_FAILURE);
        }

        //printf("I opened file\n");

        int i = 0;
        do
        {
            char cur_simb = 0;
            errno = 0;
            ret = read(fd, &cur_simb, 1);
            if (ret < 0)
            {
                perror("read error");
                exit(EXIT_FAILURE);
            }
            if (ret == 0)
                break;

            for (int j = 0; j < 8; ++j)
            {
                char mask = 0x01 << j;
                errno = 0;
                if ((mask & cur_simb) == 0)
                    ret = kill(ppid, SIGUSR1);
                else
                    ret = kill(ppid, SIGUSR2);
                if (ret < 0)
                {
                    perror("send bit to parent error");
                    exit(EXIT_FAILURE);
                }

                //printf("Child send bit\n");

                Child_glob_check = 0;
                while(Child_glob_check == 0)
                {
                    errno = 0;
                    alarm(1);
                    sigsuspend(&send_mask);
                    if (errno != EINTR)
                    {
                        perror("Not interrupt error in suspend");
                        exit(EXIT_FAILURE);
                    }
                    //printf("After child alarm\n");
                }
            }
        } while(1);
    }
    return 0;
}
