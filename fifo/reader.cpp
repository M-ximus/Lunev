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
ssize_t calcSize(const char* inFile);
char* read_file(const char* name, ssize_t* file_size);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
      printf("Error num_params\n");
      exit(EXIT_FAILURE);
    }

    ssize_t file_size = -1;
    char* buff = read_file(argv[1], &file_size);
    assert(buff != nullptr);
    assert(file_size > 0);
    
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

    char* fifo = {};
    while(true)
    {
      int writer_pid = 0;
      errno = 0;
      ssize_t ret_read = read(trans_fd, &writer_pid, sizeof(writer_pid));
      if (ret_read < 0)
      {
        perror("Read pid from transfer error\n");
        exit(EXIT_FAILURE);
      }

      fifo = fifo_name(writer_pid);

      errno = 0;
      int ret_fifo = mkfifo(fifo, 00600);
      if (ret_fifo < 0)
      {
        perror("Make named fifo error\n");
        exit(EXIT_FAILURE);
      }

      errno = 0;
      int fifo_fd = open(fifo, O_WRONLY | O_NONBLOCK);
      if (fifo_fd < 0)
      {
        perror("Named fifo can't be opened\n");
        exit(EXIT_FAILURE);
      }

////////////////////////////////////////////////////////
      sleep(5);
////////////////////////////////////////////////////////

      errno = 0;
      int ret_fcntl = fcntl(fifo_fd, F_SETFL, ~O_NONBLOCK);//without '~'?
      if (ret_fcntl < 0)
      {
        perror("Change O_NONBLOCK flag error");
        exit(EXIT_FAILURE);
      }

      errno = 0;
      int ret_val = write(fifo_fd, &file_size, sizeof(file_size));
      if (ret_val < 0)
      {
        if (errno == EPIPE)
        {
          close(fifo_fd);

          errno = 0;
          int ret_rm = remove(fifo);
          if (ret_rm < 0)
          {
            perror("Remove error\n");
            exit(EXIT_FAILURE);
          }

          continue;//TODO goto or while + continue
        }
        perror("Write to named pipe error(not connection)\n");
        exit(EXIT_FAILURE);
      }

      errno = 0;
      ret_val = write(fifo_fd, buff, file_size);
      if (ret_val < 0)
      {
        if (errno == EPIPE)
        {
          close(fifo_fd);

          errno = 0;
          int ret_rm = remove(fifo);
          if (ret_rm < 0)
          {
            perror("Remove error\n");
            exit(EXIT_FAILURE);
          }

          continue;
        }
        perror("Write to named pipe error(not connection)\n");
        exit(EXIT_FAILURE);
      }

      close(fifo_fd);
      break;
    }

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

ssize_t calcSize(const char* inFile)
{
    assert(inFile != nullptr);

    struct stat fileInfo;
    stat(inFile, &fileInfo);

    return fileInfo.st_size;
}


char* read_file(const char* name, ssize_t* file_size)
{
  if (name == nullptr || file_size == nullptr)
    return nullptr;

  errno = 0;
  int in_fd = open(name, O_RDONLY);
  if (in_fd < 0)
  {
      perror("Read file error\n");
      exit(EXIT_FAILURE);
  }

  *file_size = calcSize(name);

  errno = 0;
  char* buff = (char*) calloc(*file_size, sizeof(buff[0]));
  if (buff == nullptr || errno != 0)
  {
      perror("Calloc for file buff error\n");
      exit(EXIT_FAILURE);
  }

  errno = 0;
  ssize_t ret_val = read(in_fd, buff, (*file_size) * sizeof(buff[0]));
  if (ret_val < 0)
  {
      perror("Read from file error\n");
      exit(EXIT_FAILURE);
  }

  close(in_fd);

  return buff;
}
