#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <assert.h>
#include <wait.h>
#include <sys/types.h>
#include <limits.h>

struct msg
{
  long  msg_type;
  long int msg_num [1];
};

int snd_msg(int id, long int num_child);
int rcv_msg(int id, long int num_child);
long int give_num(const char* str_num);

int main(int argc, const char* argv[])
{
  if (argc != 2)
  {
    perror("Arg num error");
    exit(EXIT_FAILURE);
  }

  //printf("%s", argv[1]);

  long int num_child = give_num(argv[1]);

  errno = 0;
  int msg_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
  if (msg_id < 0)
  {
    perror("Creating msg queue error");
    printf("Error = %d", errno);
    exit(EXIT_FAILURE);
  }

  //printf("%ld", num_child);
//  fflush(0);

  int my_pid = 0;
  long int name = 0;
  for(long int i = 1; i <= num_child; i++)
  {
    my_pid = fork();

    if (my_pid == 0)
    {
      name = i;
      break;
    }
    else if (my_pid < 0)
    {
      perror("Trobles with creating children(((");
      exit(EXIT_FAILURE);
    }
    else
    {
      //printf("%ld", i);
      //fflush(0);
      continue;
    }
  }

  if (my_pid > 0)
  {
    //printf("I started sync children\n");
    //fflush(0);
    for(long int i = 1; i <= num_child ; i++)
    {
      snd_msg(msg_id, i);//block or error can send
      //printf("Parent called %ld\n", i);
      //fflush(0);
      rcv_msg(msg_id, num_child + 1);

      //wait(NULL);//dead lock;
    }
  }else
  {
    rcv_msg(msg_id, name);
    printf("%ld ", name);
    fflush(0);
    snd_msg(msg_id, num_child + 1);
    return 0;
  }

  errno = 0;
  int ret = msgctl(msg_id, IPC_RMID, NULL);
  if (ret < 0)
  {
    perror("Msg queue close error");
    exit(EXIT_FAILURE);
  }

  return 0;
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

int snd_msg(int id, long int num_child)
{
  assert(id >= 0);
  assert(num_child >= 0);

  msg sinc_mail = {num_child, num_child};

  errno = 0;
  int ret = msgsnd(id, &sinc_mail, sizeof(long int), MSG_NOERROR);
  if(ret < 0)
  {
    perror("Send error");
    exit(EXIT_FAILURE);
    //return -1;
  }

  //printf("Parent send mail %ld\n", num_child);
  //fflush(0);

  return 0;
}

int rcv_msg(int id, long int num_child)
{
  assert(id >= 0);
  assert(num_child >= 0);

  msg sinc_mail;// init???

  //printf("I called with num %ld\n", num_child);
  //fflush(0);

  errno = 0;
  int ret = msgrcv(id, &sinc_mail, sizeof(long int),
      num_child, MSG_NOERROR);
  if (ret < 0)
  {
    perror("Get msg error");
    exit(EXIT_FAILURE);
    //return -1;
  }

  //printf("I recieve mail %ld\n", num_child);
  //fflush(0);

  return 0;
}
