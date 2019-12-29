#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <cassert>

long int give_num(const char* str_num);

int main(int argc, char** argv) {

    if (argc != 2)
    {
        printf("Error num of arguments != 2");
        return 1;
    }

    long int n = give_num(argv[1]);

    for(long int i = 1; i <= n; i++)
    {
        //welcome = 1;

        pid_t my_pid = fork();

        if (my_pid == 0)
        {
            printf("%ld %d\n", i, getpid());
            break;
        }

        if (my_pid < 0)
        {
            printf("error\n");
            return 2;
        }
    }

    return 0;
}

long int give_num(const char* str_num)
{
    long int in_num = 0;
    char* end_string;

    errno = 0;
    in_num = strtoll(str_num, &end_string, 10);
    if ((errno != 0 && in_num == 0 ) || (errno == ERANGE && (in_num == LLONG_MAX || in_num == LLONG_MIN)))
    {
        printf("Bad string");
        return -2;
    }

    if (str_num == end_string)
    {
        printf("No number");
        return -3;
    }

    if (*end_string != '\0')
    {
        printf("Garbage after number");
        return -4;
    }

    if (in_num < 0)
    {
        printf("i want unsigned num");
        return -5;
    }

    return in_num;
}