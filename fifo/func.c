/*int check_fd(int fd1, int fd2);
{
    if (fd1 < 0 || fd2 < 0)
	return -1;

    struct stat info_fd1, info_fd2;
	
    errno = 0;
    int ret_val = fstat(fd, &info_fd1);
    if (ret_val < 0)
        return -1;

    ret_val = fstat(fd, &info_fd2);
    if (ret_val < 0)
        return -1;

    if (info_fd1.st_dev == info_fd2.st_dev && info_fd2.st_ino == info_fd1.st_ino)
        return 1;
    else
	return 0;
}*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
	char buf[4] = "";
	mkfifo("fifo", 666);
	int fd = open("fifo", O_RDWR);
	write(fd, "aaa", 3);

	//close(fd);

	fd = open("fifo", O_RDONLY | O_NONBLOCK);
	write(1, buf, read(fd, buf, 3));
	return 0;
}
