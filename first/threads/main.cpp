#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

long int give_num(const char* str_num);
void* thread_inc(void* p_num);

int Glob_num = 0;

int main(int argc, char* const argv[])
{
    if (argc != 2)
    {
        printf("No 2 params\n");
        exit(EXIT_FAILURE);
    }

    long int num_th = give_num(argv[1]);

    pthread_t* arr_thread = (pthread_t*) calloc(num_th, sizeof(*arr_thread));

    for(int i = 0; i < num_th; i++)
    {
        pthread_create(arr_thread + i, NULL, thread_inc, &num_th);
    }

    for(int i = 0; i < num_th; i++)
    {
        pthread_join
    }

    return 0;
}

void* thread_inc(void* p_num)
{
    if (p_num == nullptr)
        exit(EXIT_FAILURE);

    for(int i = 0; i < *((int*)p_num); i++)
        Glob_num++;

    return NULL;
}



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