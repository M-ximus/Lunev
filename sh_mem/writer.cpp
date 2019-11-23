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
    char* sh_mem = (char*) shmat(shm_id, NULL, 0);
    if (*sh_mem == -1)
    {
        perror("Attach mem error");
        exit(EXIT_FAILURE);
    }

    char* buff = (char*) calloc(4096, sizeof(buff[0]));

    errno = 0;
    int sem_id = semget(key, 6, IPC_CREAT | 0600);
    if (sem_id < 0)
    {
        perror("Sem get/create error");
        exit(EXIT_FAILURE);
    }

    struct sembuf sops[2] = {
        {start, 0, IPC_NOWAIT},
        {start, 1, 0}
    };// SEM_UNDO

    errno = 0;
    int ret = semop(sem_id, sops, 2);
    if (ret < 0 && errno != EAGAIN)
    {
        perror("Initialization test error");
        exit(EXIT_FAILURE);
    }

    if (ret >= 0)
    {
        printf("I'm inicializer\n");
        fflush(0);

        sembuf sops2[4] = {
            {mutex, 1, 0},
            {alive_r, 1, 0},
            {alive_w, 1, 0},
            {die, 2, 0}
          };

        errno = 0;
        ret = semop(sem_id, sops2, 4);
        if (ret < 0)
        {
            perror("Initialization sem error");
            exit(EXIT_FAILURE);
        }
    }
////////////////////////////////////////////////////////////////////////////////
    sembuf sops3[5] = {
        {die, -2, IPC_NOWAIT},
        {die, 2, 0},
        {alive_w, -1, SEM_UNDO},/*Del*/
        {full, 1, SEM_UNDO},
        {full, -1, 0}
    };

    errno = 0;
    ret = semop(sem_id, sops3, 5);
    if (ret < 0 && errno != EAGAIN)
    {
        perror("Sem enter writer error");
        exit(EXIT_FAILURE);
    }
    if (errno == EAGAIN)
    {
        perror("I died while waiting");
        exit(EXIT_FAILURE);
    }

    sembuf sops4[2] = {
      {alive_r, 0, 0},
      {die, -1, SEM_UNDO},
  };
    errno = 0;
    ret = semop(sem_id, sops4, 2);
    if (ret < 0)
    {
        perror("Sem die writer error");
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
    ssize_t ret_read = 0;
    do
    {
        errno = 0;
        ssize_t ret_read = read(fd, buff, 4096);
        if (ret_read < 0)
        {
            perror("Read file error");
            exit(EXIT_FAILURE);
        }

        sops5 = {die, 0, IPC_NOWAIT};
        errno = 0;
        ret = semop(sem_id, &sops5, 1);
        if (ret < 0)
        {
            perror("Sem check die writer error");
            exit(EXIT_FAILURE);
        }
        //exit(0);
        sembuf sops6[3] = {
            {die, 0, IPC_NOWAIT},
            {full, 0, 0},
            {mutex, -1, SEM_UNDO},
       };

        errno = 0;
        ret = semop(sem_id, sops6, 3);
        if (ret < 0)
        {
            perror("Take mutex writer error");
            exit(EXIT_FAILURE);
        }

        memset(sh_mem, '\0', 4096);
        memcpy(sh_mem, buff, 4096);
        //exit(0);
        sembuf sops7[2] = {
          {full, 1, 0},// undo to 0
          {mutex, 1, SEM_UNDO}
       };

        errno = 0;
        ret = semop(sem_id, sops7, 2);
        if (ret < 0)
        {
            perror("Return mutex writer error");
            exit(EXIT_FAILURE);
        }

        //exit(0);
        //printf("%ld\n", ret_read);
        //fflush(0);

    } while(ret_read);
////////////////////////////////////////////////////////////////////////////////

    free(buff);

    close(fd);

    errno = 0;
    sembuf sops8[2] = {
        {full, 0, 0},
        {die, 0, IPC_NOWAIT}
    };
    ret = semop(sem_id, sops8, 2);
    if (ret < 0)
    {
        perror("Wait for ending error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret = shmdt(sh_mem);
    if (ret < 0)
    {
        perror("Dettach error");
        exit(EXIT_FAILURE);
    }
    //exit(0);
    /*errno = 0;
    ret = shmctl(shm_id, IPC_RMID, NULL);
    if (ret < 0)
    {
        perror("RM_shmem error");
        exit(EXIT_FAILURE);
    }*/

    /*sembuf sops9 = {die, 1, SEM_UNDO};
    ret = semop(sem_id, &sops9, 1);
    if (ret < 0)
    {
        perror("Set die error");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    ret = semctl(sem_id, 0, IPC_RMID);
    if (ret < 0)
    {
        perror("Rm sem error");
        exit(EXIT_FAILURE);
    }*/

    return 0;
}
