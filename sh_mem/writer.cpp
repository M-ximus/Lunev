#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        perror("Num of args error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("Open file error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    key_t key = ftok("/home/max/.profile", 1);
    if (key < 0)
    {
        perror("key gen");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    int shm_id = shmget(key, 4096, IPC_CREAT | 0600);
    if (shm_id < 0)
    {
        perror("Shared memory gen error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    char* sh_mem = (char*) shmat(shm_id, NULL, 0);
    if (sh_mem == NULL)
    {
        perror("Attach mem error");
        exit(EXIT_FAILURE);
    }

    char* buff = (char*) calloc(4096, sizeof(buff[0]));

    errno = 0;
    int sem_id = semget(key, 7, IPC_CREAT | 0600);
    if (sem_id < 0)
    {
        perror("Sem get/create error");
        exit(EXIT_FAILURE);
    }

    struct sembuf sops[2];

    sops[0].sem_num = 0;
    sops[0].sem_op = 0;
    sops[0].sem_flg = IPC_NOWAIT;

    sops[1].sem_num = 0;
    sops[1].sem_op = 1;
    sops[1].sem_flg = 0;

    errno = 0;
    int ret = semop(sem_id, sops, 2);
    if (ret < 0 && errno != EAGAIN)
    {
        perror("Initialization test error");
        exit(EXIT_FAILURE);
    }

    if (ret >= 0)
    {
        sembuf sops2[5] = {
            {1, 1, 0},
            {3, 4096, 0},
            {4, 1, 0},
            {5, 1, 0},
            {6, 2, 0}
          };

        errno = 0;
        ret = semop(sem_id, sops2, 5);
        if (ret < 0)
        {
            perror("Initialization sem error");
            exit(EXIT_FAILURE);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    sembuf sops3[3] = {
        {5, -1, SEM_UNDO},/*Del*/
        {6, -2, 0},
        {6, 2, 0}
    };

    errno = 0;
    ret = semop(sem_id, sops3, 3);
    if (ret < 0)
    {
        perror("Sem enter writer error");
        exit(EXIT_FAILURE);
    }

    sembuf sops4[2] = {
      {4, 0, 0},
      {6, -1, SEM_UNDO}
  };
    errno = 0;
    ret = semop(sem_id, sops4, 2);
    if (ret < 0)
    {
        perror("Sem die writer error");
        exit(EXIT_FAILURE);
    }
    ////////////////////////////////////////////////////////////////////////////

    ssize_t ret_read = 0;
    do
    {
        errno = 0;
        ssize_t ret_read = read(fd, buff, 4096);
        if (ret < 0)
        {
            perror("Read file error");
            exit(EXIT_FAILURE);
        }

        sembuf sops5 = {6, 0, IPC_NOWAIT};
        errno = 0;
        ret = semop(sem_id, &sops5, 1);
        if (ret < 0)
        {
            perror("Sem check die writer error");
            exit(EXIT_FAILURE);
        }

        sembuf sops6[2] = {
          {3, -4096, SEM_UNDO},
          {1, -1, SEM_UNDO}
      };

        errno = 0;
        ret = semop(sem_id, sops6, 2);
        if (ret < 0)
        {
            perror("Sem mutex writer error");
            exit(EXIT_FAILURE);
        }

        memcpy(sh_mem, buff, 4096);

        sembuf sops7[2] = {
          {2, 4096, SEM_UNDO},
          {1, 1, SEM_UNDO}
      };

        errno = 0;
        ret = semop(sem_id, sops7, 2);
        if (ret < 0)
        {
            perror("Sem mutex writer error");
            exit(EXIT_FAILURE);
        }

    } while(ret_read);
////////////////////////////////////////////////////////////////////////////////

    free(buff);

    close(fd);

    errno = 0;
    ret = shmdt(sh_mem);
    if (ret < 0)
    {
        perror("Dettach error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret = shmctl(shm_id, IPC_RMID, NULL);
    if (ret < 0)
    {
        perror("RM_shmem error");
        exit(EXIT_FAILURE);
    }

    return 0;
}
