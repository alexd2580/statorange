
#include<signal.h>
#include<unistd.h>
#include<stdio.h>

void forkfunc(void)
{
  int childpid = fork();
  if(childpid == 0) //I am child
  {
    for(int i=0; i<5; i++)
    {
      sleep(5);
      kill(getppid(), SIGUSR1);
    }
    exit(0);
  }
  return;
}

void handler(int sig)
{
  printf("HANDLER\n");
  signal(SIGUSR1, handler);
}

int main(void)
{
  forkfunc();
  signal(SIGUSR1, handler);
  
  while(1)
  {
    printf("before\n");
    fflush(stdout);
    sleep(100);
    printf("after\n");
    fflush(stdout);
  }
  return 0;
}
