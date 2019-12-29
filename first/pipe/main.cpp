#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

ssize_t child(int fd[2], const char* name);
ssize_t parent(int fd[2]);
ssize_t calcSize(const char* inFile);


int main(int argc, char* argv[])
{
    if (argc != 2)
        exit(EXIT_FAILURE);

    int fd[2] = {-1, -1};

    errno = 0;
    int ret_pipe = pipe(fd);
    if (ret_pipe != 0)
    {
        printf("error - %d", errno);
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid > 0)
    {
        parent(fd);
    }
    else if (pid == 0)
    {
        child(fd, argv[1]);
    }
    else
    {
        printf("error %d\n", errno);
        exit(EXIT_FAILURE);
    }

    return 0;
}

ssize_t parent(int fd[2])
{
    assert(fd != nullptr);

    close(fd[1]);

    ssize_t size_buff = 0;
    errno = 0;
    ssize_t ret_rd = read(fd[0], &size_buff, sizeof(size_buff));
    if (ret_rd != sizeof(size_buff) || errno != 0)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    if (size_buff <= 0 || size_buff >= 0x7ffff000)
    {
        printf("size error - %ld", size_buff);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    char* buff = (char*) calloc(size_buff, sizeof(buff[0]));
    if (buff == nullptr || errno != 0)
    {
        printf("error %d, pointer %p", errno, (void*) buff);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret_rd = read(fd[0], buff, size_buff * sizeof(buff[0]));
    if (ret_rd < 0)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    close(fd[0]);

    errno = 0;
    ssize_t ret_wr = write(STDOUT_FILENO, buff, size_buff);
    if (ret_wr < 0)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    free(buff);

    wait(NULL);

    return size_buff;
}

ssize_t child(int fd[2], const char* name)
{
    assert(name != nullptr);
    assert(fd != nullptr);

    close(fd[0]);

    ssize_t size_buff = calcSize(name);
    assert(size_buff > 0);

    errno = 0;
    int file_fd = open(name, O_RDONLY);
    if (file_fd <= 0)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    char* buff = (char*) calloc(size_buff, sizeof(buff[0]));
    if (buff == nullptr)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ssize_t ret_rd = read(file_fd, buff, sizeof(buff[0]) * size_buff );
    if (errno != 0 || ret_rd < 0)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ssize_t ret_wr = write(fd[1], &size_buff, sizeof(size_buff));
    if (ret_wr <= 0 && errno != 0)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret_wr = write(fd[1], buff, size_buff *  sizeof(buff[0]));
    if (ret_wr <= 0 && errno != 0)
    {
        printf("error %d", errno);
        exit(EXIT_FAILURE);
    }

    free(buff);

    close(fd[1]);

    return size_buff;
}

ssize_t calcSize(const char* inFile)
{
    assert(inFile != nullptr);

    struct stat fileInfo;
    stat(inFile, &fileInfo);

    return fileInfo.st_size;
}