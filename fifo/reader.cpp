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


ssize_t calcSize(const char* inFile);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Error num_params\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd < 0)
    {
        printf("Error of open reading file = %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t file_size = calcSize(argv[1]);

    errno = 0;
    int ret_stat = mkfifo("fifo.p", 00600);
    if (ret_stat < 0 && errno != EEXIST)
    {
        printf("mkfifo error - %d\n", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int fifo_fd = open("fifo.p", O_WRONLY);
    if (fifo_fd < 0)
    {
        printf("Error of opening fifo = %d\n", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    char* buff = (char*) calloc(file_size, sizeof(buff[0]));
    if (buff == NULL || errno != 0)
    {
        printf("Memory error = %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ssize_t ret_val = read(in_fd, buff,file_size * sizeof(buff[0]));
    if (ret_val < 0)
    {
        printf("Read error = %d", errno);
        exit(EXIT_FAILURE);
    }

    close(in_fd);

    errno = 0;
    ret_val = write(fifo_fd, &file_size, sizeof(file_size));
    if (ret_val < 0)
    {
        printf("Write size error = %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret_val = write(fifo_fd, buff, file_size * sizeof(buff[0]));
    if (ret_val < 0)
    {
        printf("Write error = %d", errno);
        exit(EXIT_FAILURE);
    }

    close(fifo_fd);
    free(buff);

    return 0;
}

ssize_t calcSize(const char* inFile)
{
    assert(inFile != nullptr);

    struct stat fileInfo;
    stat(inFile, &fileInfo);

    return fileInfo.st_size;
}
