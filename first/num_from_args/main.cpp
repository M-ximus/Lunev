#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

int main(int argc, char* argv[]) {
    if (argc != 2)
    {
        printf("Error num of arguments != 2");
        return 1;
    }

    long int in_num = 0;
    char* end_string;

    errno = 0;
    in_num = strtoll(argv[1], &end_string, 10);
    if ((errno != 0 && in_num == 0 ) || (errno == ERANGE && (in_num == LLONG_MAX || in_num == LLONG_MIN)))
    {
        printf("Bad string");
        return 2;
    }

    if (argv[1] == end_string)
    {
        printf("No number");
        return 3;
    }

    if (*end_string != '\0')
    {
        printf("Garbage after number");
        return 4;
    }

    if (in_num < 0)
    {
        printf("i want unsigned num");
        return 5;
    }

    for (long int i = 0; i <= in_num; i++)
        printf("%ld\n %d", i, max);

    return 0;
}