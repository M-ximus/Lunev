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
  int ret_val = write(trans_fd, &my_pid, sizeof(my_pid));
  if (ret_val < 0)
  {
    perror("Write pid to transfer fifo error\n");
    exit(EXIT_FAILURE);
  }

  int fifo_fd = -1;
  for(int i = 0; i < 5; i++)
  {
/////////////////////////////////////////////////////////
    sleep(5);
/////////////////////////////////////////////////////////

    errno = 0;
    fifo_fd = open(named_fifo, O_RDONLY | O_NONBLOCK);
    if (fifo_fd < 0 && errno == EACCES)
      continue;
    if (fifo_fd < 0)
    {
      perror("Open named fifo failed(not for sync with reader\n");
      exit(EXIT_FAILURE);
    }

    break;
  }

  int ret_fcntl = fcntl(fifo_fd, F_SETFL, ~O_NONBLOCK);
  if (ret_fcntl < 0)
  {
    perror("Change O_NONBLOCK flag error\n");
    exit(EXIT_FAILURE);
  }

  ssize_t size_buff = 0;
  errno = 0;
  ret_val = read(fifo_fd, &size_buff, sizeof(size_buff));
  if (ret_val <= 0 || errno != 0)
  {
    perror("Read size error\n");
    exit(EXIT_FAILURE);
  }

  errno  = 0;
  char* buff = (char*) calloc(size_buff, sizeof(buff[0]));
  if (buff == nullptr)
  {
    perror("Calloc fail\n");
    exit(EXIT_FAILURE);
  }

  errno = 0;
  ret_val = read(fifo_fd, buff, size_buff * sizeof(buff[0]));
  if (ret_val < 0 || errno != 0)
  {
    perror("Read size error\n");
    exit(EXIT_FAILURE);
  }

  close(fifo_fd);

  print_text(buff, size_buff);
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
