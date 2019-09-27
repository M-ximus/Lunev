//
// Created by max on 26.09.19.
//

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

int main()
{
    errno = 0;
    int ret_stat = mkfifo("fifo.p", 00600);
    if (ret_stat < 0 && errno != EEXIST)
    {
        printf("mkfifo error - %d\n", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int fifo_fd = open("fifo.p", O_RDONLY);
    if (fifo_fd < 0)
    {
        printf("Open fifo error = %d", errno);
        exit(EXIT_FAILURE);
    }

    size_t size_buff = 0;
    errno = 0;
    ssize_t ret_val = read(fifo_fd, &size_buff, sizeof(size_buff));
    if (ret_val < 0)
    {
        printf("Read size error = %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    char* buff = (char*) calloc(size_buff, sizeof(buff[0]));
    if (buff == NULL || errno != 0)
    {
        printf("Memory error = %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret_val = read(fifo_fd, buff, size_buff * sizeof(buff[0]));
    if (ret_val < 0)
    {
        printf("Read error = %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret_val = write(STDOUT_FILENO, buff, size_buff * sizeof(buff[0]));
    if (ret_val < 0)
    {
        printf("Print error = %d", errno);
        exit(EXIT_FAILURE);
    }

    free(buff);
    close(fifo_fd);

    return 0;
}
