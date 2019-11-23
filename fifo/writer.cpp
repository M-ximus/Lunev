//
// Created by max on 26.09.19.
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

char* fifo_name(int pid);
ssize_t print_text(const char* buff, size_t size_buff);

int main()
{
    errno = 0;
    char* buff = (char*) calloc(4096, sizeof(buff[0]));
    if (buff == NULL)
    {
        perror("Allocate buff error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int ret_stat = mkfifo("transfer.p", 00600);
    if (ret_stat < 0 && errno != EEXIST)
    {
        perror("Creation transfer error(not EEXIST)\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int trans_fd = open("transfer.p", O_WRONLY);
    if (trans_fd < 0)
    {
        perror("Open transfer error\n");
        exit(EXIT_FAILURE);
    }

    int my_pid = getpid();

    char* named_fifo = fifo_name(my_pid);
    printf("%s\n", named_fifo);

    errno = 0;
    int ret_fifo = mkfifo(named_fifo, 00600);
    if (ret_fifo < 0)
    {
      perror("Make named fifo error\n");
      exit(EXIT_FAILURE);
    }


    errno = 0;
    int fifo_fd = open(named_fifo, O_RDONLY | O_NONBLOCK);
    if (fifo_fd < 0)
    {
        perror("Open named fifo failed\n");
        exit(EXIT_FAILURE);
    }


    errno = 0;
    int ret_val = write(trans_fd, &my_pid, sizeof(my_pid));
    if (ret_val < 0)
    {
        perror("Write pid to transfer fifo error\n");
        exit(EXIT_FAILURE);
    }

    /*int ret_fcntl = fcntl(fifo_fd, F_SETFL, ~O_NONBLOCK);
    if (ret_fcntl < 0)
    {
        perror("Change O_NONBLOCK flag error\n");
        exit(EXIT_FAILURE);
    }*/

    int i = 0;
    for (; i < 5; ++i)
    {
        errno = 0;
        ret_val = read(fifo_fd, buff, 4096 * sizeof(buff[0]));
        if (ret_val < 0 || errno != 0)
        {
            perror("Connection read error\n");
            exit(EXIT_FAILURE);
        }
        if (ret_val > 0)
        {
            print_text(buff, ret_val);
            break;
        }
////////////////////////////////////////////////////////////////////////////////
        sleep(5);
////////////////////////////////////////////////////////////////////////////////
    }
    if (i == 5)
    {
        printf("I can't connect\n");
        exit(EXIT_FAILURE);
    }

    int ret_fcntl = fcntl(fifo_fd, F_SETFL, O_RDONLY);
    if (ret_fcntl < 0)
    {
        perror("Change O_NONBLOCK flag error\n");
        exit(EXIT_FAILURE);
    }

    do
    {
        errno = 0;
        ret_val = read(fifo_fd, buff, 4096 * sizeof(buff[0]));
        if (ret_val < 0 || errno != 0)
        {
            perror("Read from buff error\n");
            exit(EXIT_FAILURE);
        }

        print_text(buff, ret_val);
    } while(ret_val > 0);

    close(fifo_fd);

    free(buff);

    errno = 0;
    int ret_rm = remove(named_fifo);
    if (ret_rm < 0)
    {
        perror("Remove error\n");
        exit(EXIT_FAILURE);
    }

    free(named_fifo);

    return 0;
}

char* fifo_name(int pid)
{

  errno = 0;
  char* pid_buff = (char*) calloc(17, sizeof(pid_buff[0]));//fifoi32.p\0
  if (errno != 0)
  {
    printf("pid mem error = %d", errno);
    exit(EXIT_FAILURE);
  }

  strcpy(pid_buff, "fifo");
  sprintf((pid_buff + 4), "%d", pid);
  strcat(pid_buff, ".p");

  return pid_buff;
}

ssize_t print_text(const char* buff, size_t size_buff)
{
  if (buff == nullptr)
    return -1;

  errno = 0;
  ssize_t ret_val = write(STDOUT_FILENO, buff, size_buff * sizeof(*buff));
  if (ret_val < 0)
  {
      perror("STDOUT print error\n");
      exit(EXIT_FAILURE);
  }

  return ret_val;
}
