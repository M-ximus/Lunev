#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>

const size_t Chld_buff_size = 128000;
const size_t Max_buff_size = 128000;

struct chld_info
{
    int to_parent_fd[2];
    int from_parent_fd[2];
    int pid_chld;
};

struct connection
{
    int           fd_wr;//
    int          fd_rd;//
    size_t  buff_size;//
    char*  write_end;//
    char*  read_end;//
    char*     buff;//
    char*     end;//
    size_t empty;//
    size_t full;//
};

long int give_num(const char* str_num);
int child(const char* file_name, chld_info* links, long int number, long int num_chld);
int parent(chld_info* links, long int num_chld, int max_fd);

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("I need 2 params\n");
        exit(EXIT_FAILURE);
    }

    long int num_chld = give_num(argv[1]);
    if (num_chld < 0)
    {
        printf("Number of children is incorrect\n");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    struct chld_info* links = (chld_info*) calloc(num_chld, sizeof(links[0]));
    if (links == NULL)
    {
        perror("Memory error");
        exit(EXIT_FAILURE);
    }

    int max_fd = 0;
    int chpid  = 0;
    int ret    = 0;
    long int i = 0;
    for(; i < num_chld; i++)
    {
        if (i != num_chld - 1)
        {
            errno = 0;
            ret = pipe2(links[i].to_parent_fd, O_NONBLOCK);
            if (ret < 0)
            {
                perror("Create chld to parent pipe error");
                exit(EXIT_FAILURE);
            }

            if (links[i].to_parent_fd[0] > max_fd)
                max_fd = links[i].to_parent_fd[0];
            if (links[i].to_parent_fd[1] > max_fd)
                max_fd = links[i].to_parent_fd[1];
        }

        if (i != 0)
        {
            errno = 0;
            ret = pipe2(links[i].from_parent_fd, O_NONBLOCK);
            if (ret < 0)
            {
                perror("Create parent to child pipe error");
                exit(EXIT_FAILURE);
            }
            //printf("From Parent %ld write_fd = %d, read_fd = %d\n", i, links[i].from_parent_fd[1], links[i].from_parent_fd[0]);
            if (links[i].from_parent_fd[0] > max_fd)
                max_fd = links[i].from_parent_fd[0];
            if (links[i].from_parent_fd[1] > max_fd)
                max_fd = links[i].from_parent_fd[1];
        }

        chpid = fork();
        if (chpid > 0)
        {
            links[i].pid_chld = chpid;
            if (i != 0)
                close(links[i].from_parent_fd[0]);
            if (i != num_chld - 1)
                close(links[i].to_parent_fd[1]);
            continue;
        }

        if (chpid == 0)
        {
            for(long j = 0; j < i; j++)
                close(links[j].from_parent_fd[1]);
            break;
        }

        if (chpid < 0)
        {
            perror("I can't have children");
            exit(EXIT_FAILURE);
        }
    }

    if (chpid > 0)
        parent(links, num_chld, max_fd + 1);
    else
        child(argv[2], links + i, i, num_chld);

    return 0;
}

//------------------------------------------------------------------------------
int child(const char* file_name, chld_info* links, long int number, long int num_chld)
{
    assert(file_name != NULL);
    assert(links != NULL);

    if (number > 0 && number < num_chld - 1)
    {
        close(links->to_parent_fd[0]);
        close(links->from_parent_fd[1]);
    }

    if (number == 0)
    {
        close(links->to_parent_fd[0]);

        errno = 0;
        links->from_parent_fd[0] = open(file_name, O_RDONLY);
        if (links->from_parent_fd < 0)
        {
            perror("Open in file error");
            exit(EXIT_FAILURE);
        }
    }

    if (number == num_chld - 1)
    {
        close(links->from_parent_fd[1]);

        //links->to_parent_fd[1] = open("out.txt", O_WRONLY | O_CREAT, 0666);//STDOUT_FILENO;
        links->to_parent_fd[1] = STDOUT_FILENO;
    }

    ssize_t ret = 0;
    if (number != 0)
    {
        errno = 0;
        ret = fcntl(links->from_parent_fd[0], F_SETFL, O_RDONLY);//////////////////
        if (ret < 0)
        {
            perror("Set zero flag in child error");
            exit(EXIT_FAILURE);
        }
    }

    if (number != num_chld - 1)
    {
        errno = 0;
        ret = fcntl(links->to_parent_fd[1], F_SETFL, O_WRONLY);//////////////////
        if (ret < 0)
        {
            perror("Set zero flag in child error");
            exit(EXIT_FAILURE);
        }
    }

    errno = 0;
    char* buff = (char*) calloc(Chld_buff_size, sizeof(char));
    if (buff == NULL)
    {
        perror("First child mem alloc");
        exit(EXIT_FAILURE);
    }

    ssize_t ret_read = 0;
    char* read_end  = buff;
    char* write_end = buff;
    char* const end = buff + Chld_buff_size;
    size_t empty = Chld_buff_size;
    size_t full = 0;
    do {
        //printf("/////////////////////////////////%d/////////////////\n", getpid());
        //if (number == 2)
            //sleep(1);
        if (empty > 0)
        {
            errno = 0;
            ret_read = read(links->from_parent_fd[0], write_end, empty);
            if (ret < 0)
            {
                perror("Read from file error");
                exit(EXIT_FAILURE);
            }
            if (write_end + ret_read == end)
            {
                write_end = buff;
                full += ret_read;
                empty = read_end - write_end;
            }
            else
            {
                if (write_end >= read_end)
                {
                    write_end += ret_read;
                    empty -= ret_read;
                    full += ret_read;
                }
                else
                {
                    write_end += ret_read;
                    empty -= ret_read;
                }
            }
        }

        errno = 0;
        ret = write(links->to_parent_fd[1], read_end, full);
        if (ret < 0)
        {
            perror("Child can't write to pipe");
            exit(EXIT_FAILURE);
        }
        if (read_end + ret == end)
        {
            read_end = buff;
            full = write_end - read_end;
            empty += ret;
        }
        else
        {
            if (read_end > write_end)
            {
                read_end += ret;
                empty += ret;
                full -= ret;
            }
            else
            {
                read_end += ret;
                full -= ret;
            }
        }
    } while(full > 0 || ret_read > 0);

    close(links->from_parent_fd[0]);
    close(links->to_parent_fd[1]);
    free(buff);

    exit(EXIT_SUCCESS);
}

//------------------------------------------------------------------------------
int parent(chld_info* links, long int num_chld, int max_fd)
{
    assert(links != NULL);

    errno = 0;
    connection* nodes = (connection*) calloc(num_chld - 1, sizeof(nodes[0]));
    if (nodes == NULL)
    {
        perror("Connection array alloc error");
        exit(EXIT_FAILURE);
    }

    for(long int i = 0; i < num_chld - 1; i++)
    {
        if (i < num_chld - 6)
            nodes[i].buff_size = Max_buff_size;
        else
        {
            nodes[i].buff_size = 1000;
            for(long j = 0; j < (num_chld - 2) - i; j++)
                nodes[i].buff_size *= 3;
        }
        //printf("I'm %ld and my buff %ld\n", i, nodes[i].buff_size);

        errno = 0;
        nodes[i].buff = (char*) calloc(nodes[i].buff_size, sizeof(nodes[i].buff[0]));
        if (nodes[i].buff == NULL)
        {
            perror("Alloc connection buff error");
            exit(EXIT_FAILURE);
        }

        nodes[i].end       = nodes[i].buff + nodes[i].buff_size;
        nodes[i].write_end = nodes[i].buff;
        nodes[i].read_end  = nodes[i].buff;

        nodes[i].empty     = nodes[i].buff_size;
        nodes[i].full      = 0;

        nodes[i].fd_rd     = links[i].to_parent_fd[0];

        nodes[i].fd_wr     = links[i + 1].from_parent_fd[1];
    }

    long int curr_alive = 0;
    do
    {
        fd_set readfds,writefds;

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        for(long int i = curr_alive; i < num_chld - 1; i++)
        {
            if (nodes[i].fd_rd >= 0 && nodes[i].empty > 0)
                FD_SET(nodes[i].fd_rd, &readfds);
            if (nodes[i].fd_wr >= 0 && nodes[i].full > 0)
                FD_SET(nodes[i].fd_wr, &writefds);
        }

        errno = 0;
        int num_ready = select(max_fd, &readfds, &writefds, NULL, NULL);
        if (num_ready < 0 && errno != EINTR)
        {
            perror("Select error");
            exit(EXIT_FAILURE);
        }


        for(long int i = curr_alive; i < num_chld - 1; i++)
        {
            //printf("%ld buff: size = %ld, empty = %ld, full = %ld. Current read_end = %ld. Current write_end = %ld\n", i, nodes[i].buff_size, nodes[i].empty, nodes[i].full, nodes[i].read_end - nodes[i].buff, nodes[i].write_end - nodes[i].buff);

            if (FD_ISSET(nodes[i].fd_rd, &readfds) && nodes[i].empty > 0)
            {
                errno = 0;
                ssize_t ret_read = read(nodes[i].fd_rd, nodes[i].read_end, sizeof(nodes[i].buff[0]) * nodes[i].empty);
                if (ret_read < 0)
                {
                    perror("Read from child error");
                    exit(EXIT_FAILURE);
                }
                if (ret_read == 0)
                {
                    close(nodes[i].fd_rd);
                    nodes[i].fd_rd = -1;
                    //break;//////////////////////////////////////////////////////
                }

                if (nodes[i].read_end + ret_read == nodes[i].end)
                {
                    nodes[i].read_end = nodes[i].buff;
                    nodes[i].full += ret_read;
                    nodes[i].empty = nodes[i].write_end - nodes[i].read_end;
                }
                else
                {
                    if (nodes[i].read_end >= nodes[i].write_end)
                        nodes[i].full += ret_read;
                    nodes[i].empty -= ret_read;
                    nodes[i].read_end += ret_read;
                }
            }

            if (FD_ISSET(nodes[i].fd_wr, &writefds) && nodes[i].full > 0)
            {
                errno = 0;
                ssize_t ret_write = write(nodes[i].fd_wr, nodes[i].write_end, sizeof(nodes[i].buff[0]) * nodes[i].full);
                if (ret_write < 0)
                {
                    perror("Write to child error");
                    exit(EXIT_FAILURE);
                }

                if (nodes[i].write_end + ret_write == nodes[i].end)
                {
                    nodes[i].write_end = nodes[i].buff;
                    nodes[i].empty += ret_write;
                    nodes[i].full = nodes[i].read_end - nodes[i].write_end;
                }
                else
                {
                    if (nodes[i].write_end >= nodes[i].read_end)
                        nodes[i].empty += ret_write;
                    nodes[i].full -= ret_write;
                    nodes[i].write_end += ret_write;
                }
            }

            if (nodes[i].fd_rd == -1 && nodes[i].full == 0 && nodes[i].fd_wr != -1)
            {
                close(nodes[i].fd_wr);
                nodes[i].fd_wr = -1;
                if (curr_alive != i)
                {
                    printf("Child %ld was killed\n", i);
                    fflush(0);
                    exit(EXIT_FAILURE);
                }
                curr_alive++;
                free(nodes[i].buff);
            }
        }
    } while(curr_alive < num_chld - 1);

    for(long i = 0; i < num_chld -1; i++)
    {
        if (nodes[i].fd_wr != -1)
            close(nodes[i].fd_wr);
        if (nodes[i].fd_rd != -1)
            close(nodes[i].fd_rd);
    }


    for(long int i = 0; i < num_chld; i++)
        {
            errno  = 0;
            int ret = waitpid(links[i].pid_chld, NULL, 0);
            if (ret < 0)
            {
                perror("waitpid()");
                exit(EXIT_FAILURE);
            }
        }

    exit(EXIT_SUCCESS);
}

//------------------------------------------------------------------------------
long int give_num(const char* str_num)
{
    long int in_num = 0;
    char *end_string;

    errno = 0;
    in_num = strtoll(str_num, &end_string, 10);
    if ((errno != 0 && in_num == 0) || (errno == ERANGE && (in_num == LLONG_MAX || in_num == LLONG_MIN))) {
        printf("Bad string");
        return -2;
    }

    if (str_num == end_string) {
        printf("No number");
        return -3;
    }

    if (*end_string != '\0') {
        printf("Garbage after number");
        return -4;
    }

    if (in_num < 0) {
        printf("i want unsigned num");
        return -5;
    }

    return in_num;
}

/*
if (FD_ISSET(nodes[i].fd_wr, &writefds) && nodes[i].full > 0)
{
    errno = 0;
    ssize_t ret_write = write(nodes[i].fd_wr, nodes[i].write_end, sizeof(nodes[i].buff[0]) * nodes[i].full);
    if (ret_write < 0)
    {
        perror("Write to child error");
        exit(EXIT_FAILURE);
    }

    if (nodes[i].write_end + ret_write == nodes[i].end)
    {
        nodes[i].write_end = nodes[i].buff;
        nodes[i].empty += ret_write;
        nodes[i].full = nodes[i].read_end - nodes[i].write_end;
    }
    else
    {
        if (nodes[i].write_end >= nodes[i].read_end)
            nodes[i].empty += ret_write;
        nodes[i].full -= ret_write;
        nodes[i].write_end += ret_write;
    }

    printf("I push info into %ld child %ldbytes\n", i + 1, ret_write);
    fflush(0);
}*/


/*
if (read_end == write_end && full != 0)
{
    errno = 0;
    ret = write(links->chld_parent_fd[1], read_end, full * sizeof(char));
    if (ret < 0)
    {
        perror("Write to the first pipe error");
        exit(EXIT_FAILURE);
    }
    if (read_end + ret == end)
    {
        read_end == buff;
        full = write_end - read_end;
        empty += ret;
    }
    else
    {
        read_end += ret;
        empty += ret;
        full -= ret;
    }
}
else
{








////////////////////////////////////////////////////////////////////////////////
for(long int i = 0; i < num_chld; i++)
{
    if (i < num_chld - 4)
        links[i].buff_size = 128000;
    else
    {

        links[i].buff_size = 1000;
        for(int j = 0; j < (num_chld - i); j++)
            links[i].buff_size *= 3;
    }

    errno = 0;
    links[i].buff = (char*) calloc(links[i].buff_size, sizeof(char));
    if (links[i].buff == NULL)
    {
        perror("Parent memory error");
        exit(EXIT_FAILURE);
    }
    links[i].write_end = links[i].buff;
    links[i].read_end  = links[i].buff;
    links[i].end       = links[i].buff + links[i].buff_size;
    links[i].empty     = links[i].buff_size;
    links[i].full      = 0;
}

fd_set readfds, writefds;
FD_ZERO(&readfds);
FD_ZERO(&writefds);

for(long int i = 0; i < num_chld; i++)
{
    if (i != 0)
    {
        close(links[i].parent_chld_fd[0]);
        FD_SET(links[i].parent_chld_fd[1], &writefds);
    }
    if (i != num_chld - 1)
    {
        close(links[i].chld_parent_fd[1]);
        FD_SET(links[i].chld_parent_fd[0], &readfds);
    }
}

do {

    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    for(long int i = 0; i < num_chld; i++)
    {
        if (i != 0)
        {
            close(links[i].parent_chld_fd[0]);
            FD_SET(links[i].parent_chld_fd[1], &writefds);
        }
        if (i != num_chld - 1)
        {
            close(links[i].chld_parent_fd[1]);
            FD_SET(links[i].chld_parent_fd[0], &readfds);
        }
    }

    struct timeval slp;
    slp.tv_sec = 0;
    slp.tv_usec = 500000;

    errno = 0;
    ret = select(max_fd + 1, &readfds, &writefds, NULL, &slp);
    if (ret < 0)
    {
        perror("Wait somebody error");
        exit(EXIT_FAILURE);
    }
    if (ret == 0)
    {
        printf("I can't wait\n");
        break;
    }

    for(int i = 0; i < num_chld; i++)// checks curr died chld
    {
        if (FD_ISSET(links[i].chld_parent_fd[0], &readfds) && links[i].empty > 0)
        {
            errno = 0;
            ssize_t ret_read = read(links[i].chld_parent_fd[0], links[i].write_end, links[i].empty * sizeof(char));
            if(ret_read < 0)
            {
                perror("Parent read error");
                exit(EXIT_FAILURE);
            }

            if (ret_read == 0)
                break;

            if (links[i].write_end + ret_read == links[i].end)
            {
                links[i].write_end = links[i].buff;
                links[i].full += ret_read;
                links[i].empty = links[i].read_end - links[i].write_end;
            }
            else
            {
                if (links[i].write_end > links[i].read_end)
                    links[i].full += ret_read;

                links[i].empty -= ret_read;
                links[i].write_end += ret_read;
            }
        }
        if (FD_ISSET(links[i].parent_chld_fd[1], &writefds) && links[i].full > 0)
        {
            errno = 0;
            ssize_t ret_write = write(links[i].parent_chld_fd[1], links[i].read_end, links[i].full * sizeof(char));
            if (ret_write < 0)
            {
                perror("Write from parent to pipe error");
                exit(EXIT_FAILURE);
            }
            if (links[i].read_end + ret_write == links[i].end)
            {
                links[i].read_end = links[i].buff;
                links[i].full = links[i].write_end - links[i].read_end;
                links[i].empty += ret_write;
            }
            else
            {
                if (links[i].read_end > links[i].write_end)
                    links[i].empty += ret_write;
                links[i].read_end += ret_write;
                links[i].full -= ret_write;
            }
        }
    }

} while();
*/
