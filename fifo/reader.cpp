//
// Created by max on 26.09.19.
//

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>

char* fifo_name(int pid);
//char* read_file(const char* name, ssize_t* file_size);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
      printf("Error num_params\n");
      exit(EXIT_FAILURE);
    }

    errno = 0;
    char* buff = (char*) calloc(4096, sizeof(buff[0]));
    if (buff == NULL)
    {
        perror("Allocate buff error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int file_fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if (file_fd < 0)
    {
        perror("Open file error");
        exit(EXIT_FAILURE);
    }
    //printf("I opened\n");

    errno = 0;
    int ret_stat = mkfifo("transfer.p", 00600);
    if (ret_stat < 0 && errno != EEXIST)
    {
      perror("Creation transfer error(not EEXIST)\n");
      exit(EXIT_FAILURE);
    }

    errno = 0;
    int trans_fd = open("transfer.p", O_RDONLY);
    if (trans_fd < 0)
    {
      perror("Open transfer error\n");
      exit(EXIT_FAILURE);
    }

    //char* fifo = nullptr;

    int writer_pid = 0;
    errno = 0;
    ssize_t ret_read = read(trans_fd, &writer_pid, sizeof(writer_pid));
    if (ret_read <= 0)
    {
      perror("Read pid from transfer error\n");
      exit(EXIT_FAILURE);
    }

    fifo = fifo_name(writer_pid);
    //printf("%s\n", fifo);


    errno = 0;
    int fifo_fd = open(fifo, O_WRONLY | O_NONBLOCK);
    if (fifo_fd < 0)
    {
        perror("Named fifo can't be opened\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int ret_fcntl = fcntl(fifo_fd, F_SETFL, O_WRONLY);//without '~'?
    if (ret_fcntl < 0)
    {
        perror("Change O_NONBLOCK flag error");
        exit(EXIT_FAILURE);
    }

    do
    {
        errno = 0;
        ret_read = read(file_fd, buff, 4096 * sizeof(buff[0]));
        if (ret_read < 0)
        {
            perror("Read from file error");
            exit(EXIT_FAILURE);
        }

        errno = 0;
        ssize_t ret_write = write(fifo_fd, buff, ret_read * sizeof(buff[0]));
        if (ret_write <= 0 && errno == EPIPE)
        {
            perror("Died transfer fifo");
            exit(EXIT_FAILURE);
        }
        if (ret_write < 0)
        {
            perror("Write to transfer fifo error");
            exit(EXIT_FAILURE);
        }
    } while(ret_read > 0);

    close(trans_fd);

    free(fifo);
    free(buff);

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
