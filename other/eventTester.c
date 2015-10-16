
#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/wait.h>
#include"die.h"
#include"i3-ipc.h"
#include"i3-ipc-constants.h"

#define getSocket "i3 --get-socketpath"
#define shell_file_loc "/bin/sh"

typedef char const cchar;

volatile sig_atomic_t die;

ssize_t execute(cchar* command, cchar* arg1, cchar* arg2, char* buffer, size_t bufSize)
{
  int fd[2];
  pipe(fd);
  int childpid = fork();
  if(childpid == -1) //Fail
  {
     fprintf(stderr, "FORK failed");
     return 0;
  } 
  else if(childpid == 0) //I am child
  {
    close(1); //stdout
    dup2(fd[1], 1);
    close(fd[0]);
    execlp(shell_file_loc, shell_file_loc, "-c", command, arg1, arg2, (char*)NULL);
    //child has been replaced by shell command
  }

  wait(NULL);
  errno = EINTR;
  ssize_t readBytes = 0;
  ssize_t n = 0;
  while(errno == EAGAIN || errno == EINTR)
  {
    errno = 0;
    n = read(fd[0], buffer, bufSize-1);
    if(n == 0)
      break;
    readBytes += n;
  }
  buffer[readBytes] = '\0';
  return readBytes;
}

void handleEvent(uint32_t type, char* response)
{
  switch(type)
  {
  case I3_INVALID_TYPE:
    die = 1;
    break;
  case I3_IPC_EVENT_MODE:
    fprintf(stderr, "MODE EVENT\n");
    if(response != NULL) fprintf(stderr, "%s\n\n", response);
    break;
  case I3_IPC_EVENT_WINDOW:
    fprintf(stderr, "WINDOW EVENT\n");
    if(response != NULL) fprintf(stderr, "%s\n\n", response);
    break;
  case I3_IPC_EVENT_WORKSPACE:
    fprintf(stderr, "WORKSPACE EVENT\n");
    if(response != NULL) fprintf(stderr, "%s\n\n", response);
    break;
  case I3_IPC_EVENT_OUTPUT:
    fprintf(stderr, "OUTPUT EVENT\n");
    if(response != NULL) fprintf(stderr, "%s\n\n", response);
    break;
  default:
    fprintf(stderr, "Type %d\n", type);
    break;
  }

  if(response != NULL)
    free(response);
}

/******************************************************************************/

int main(void)
{
  char path[100] = { 0 };
  execute(getSocket, NULL, NULL, path, 100);
  char* p=path;
  while(*p != '\n') p++;
  *p = '\0';

  int push_socket = init_socket(path);
  
  char abonnements[] = "[\"workspace\",\"mode\",\"output\",\"window\"]";
  sendMessage(push_socket, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, abonnements);
  uint32_t type;
  char* response = readMessage(push_socket, &type);
  handleEvent(type, response);
  
  while(!die)
  {
    response = readMessage(push_socket, &type);
    handleEvent(type, response);
  }
  
  return 0; 
}

