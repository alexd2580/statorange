
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include"jsonParser.h"

ssize_t readall(int fd, void* buf, size_t count)
{
  size_t raed = 0;
  ssize_t n = 0;

  while(raed < count)
  {
    n = read(fd, (char*)buf + raed, count - raed);
    if(n <= 0)
    {
      if(errno == EINTR || errno == EAGAIN)
        continue;
      return n;
    }
    raed += (size_t)n;
  }
  return (ssize_t)raed;
}

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  char* buffer = (char*)malloc(100000*sizeof(char));
  readall(STDIN_FILENO, buffer, 100000);
  JSONSomething* json = parseJSON(buffer);
  printJSON(json);
  freeJSON(json);
  free(buffer);
  return 0;
}
