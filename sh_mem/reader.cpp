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

enum {
    start   = 0,
    mutex   = 1,
    full    = 2,
    alive_r = 3,
    alive_w = 4,
    die     = 5
};

int main()
{
    errno = 0;
    key_t key = ftok("/home/max/.profile", 1);//
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
    char* sh_mem = (char*) shmat(shm_id, NULL, 0);//
    if (sh_mem == NULL)
    {
        perror("Attach mem error");
        exit(EXIT_FAILURE);
    }

    char* buff = (char*) calloc(4096, sizeof(buff[0]));

    errno = 0;
    int sem_id = semget(key, 6, IPC_CREAT | 0600);
    if (sem_id < 0 )
    {
        perror("Sem get/create error");
        exit(EXIT_FAILURE);
    }

    struct sembuf sops[6] = {
        {start, 0, IPC_NOWAIT},
        {start, 1, 0},
        {mutex, 1, 0},
        {alive_r, 1, 0},
        {alive_w, 1, 0},
        {die, 2, 0}
    };// SEM_UNDO

    errno = 0;
    int ret = semop(sem_id, sops, 2);
    if (ret < 0 && errno != EAGAIN)
    {
        perror("Initialization test error");
        exit(EXIT_FAILURE);
    }
////////////////////////////////////////////////////////////////////////////////
    sembuf sops3[3] = {
        {die, -2, IPC_NOWAIT},
        {die, 2, 0},
        {alive_r, -1, SEM_UNDO},
    };// maybe kill

    errno = 0;
    ret = semop(sem_id, sops3, 3);
    if (ret < 0 && errno != EAGAIN)
    {
        perror("Reader sem init error");
        exit(EXIT_FAILURE);
    }
    if (errno == EAGAIN)
    {
        perror("I deied while waiting");
        exit(EXIT_FAILURE);
    }

    sembuf sops4[2] = {
      {alive_w, 0, 0},
      {die, -1, SEM_UNDO}
    };
    errno = 0;
    ret = semop(sem_id, sops4, 2);
    if (ret < 0)
    {
        perror("Read die init error");
        exit(EXIT_FAILURE);
    }
////////////////////////////////////////////////////////////////////////////////
    errno = 0;
    sembuf sops5 = {die, 0, 0};
    ret = semop(sem_id, &sops5, 1);
    if (ret < 0)
    {
        perror("Wait love error");
        exit(EXIT_FAILURE);
    }
////////////////////////////////////////////////////////////////////////////////
    ssize_t ret_write = 0;
    ssize_t ret_read = 0;
    do
    {
        sops5 = {die, 0, IPC_NOWAIT};
        errno = 0;
        ret = semop(sem_id, &sops5, 1);
        if (ret < 0 && errno != EAGAIN)
        {
            perror("Die brother error");
            exit(EXIT_FAILURE);
        }
        if (ret < 0 && errno == EAGAIN)
            break;

        sembuf sops6[3] = {
            {die, 0, IPC_NOWAIT},
            {full, -1, 0},
            {mutex, -1, SEM_UNDO},
        };

        errno = 0;
        ret = semop(sem_id, sops6, 3);
        if (ret < 0 && errno != EAGAIN)
        {
            perror("Take mutex reader error");
            exit(EXIT_FAILURE);
        }
        if (ret < 0 && errno == EAGAIN)
            break;

        memcpy(buff, sh_mem, 4096);//

        sembuf sops7 = {mutex, 1, SEM_UNDO};
        errno = 0;
        ret = semop(sem_id, &sops7, 1);
        if (ret < 0)
        {
            perror("Set mutex reader error");
            exit(EXIT_FAILURE);
        }

        errno = 0;
        ret_write = write(STDOUT_FILENO, buff, 4096);
        if (ret_write < 0)
        {
            perror("Out write error");
            exit(EXIT_FAILURE);
        }
        //printf("%ld\n", ret_write);
        //fflush(0);
    } while(ret_write);

    free(buff);

    errno = 0;
    ret = shmdt(sh_mem);//
    if (ret < 0)
    {
        perror("Dettach error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret = shmctl(shm_id, IPC_RMID, NULL);//
    if (ret < 0)
    {
        perror("RM_shmem error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret = semctl(sem_id, 0, IPC_RMID);//
    if (ret < 0)// && != RM
    {
        perror("Rm sem error");
        exit(EXIT_FAILURE);
    }

    return 0;
}
