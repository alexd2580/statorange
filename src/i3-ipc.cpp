
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cerrno>

#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>

#include "i3-ipc-constants.hpp"

extern volatile sig_atomic_t die;

using namespace std;

#define CASE_RETURN(l)                                                         \
  case l:                                                                      \
    return #l;

string ipc_type_to_string(unsigned int type)
{
  switch(type)
  {
    CASE_RETURN(I3_IPC_REPLY_TYPE_TREE)
    CASE_RETURN(I3_IPC_REPLY_TYPE_MARKS)
    CASE_RETURN(I3_IPC_REPLY_TYPE_COMMAND)
    CASE_RETURN(I3_IPC_REPLY_TYPE_OUTPUTS)
    CASE_RETURN(I3_IPC_REPLY_TYPE_VERSION)
    CASE_RETURN(I3_IPC_REPLY_TYPE_SUBSCRIBE)
    CASE_RETURN(I3_IPC_REPLY_TYPE_WORKSPACES)
    CASE_RETURN(I3_IPC_REPLY_TYPE_BAR_CONFIG)
    CASE_RETURN(I3_IPC_EVENT_MODE)
    CASE_RETURN(I3_IPC_EVENT_OUTPUT)
    CASE_RETURN(I3_IPC_EVENT_WINDOW)
    CASE_RETURN(I3_IPC_EVENT_BINDING)
    CASE_RETURN(I3_IPC_EVENT_WORKSPACE)
    CASE_RETURN(I3_IPC_EVENT_BARCONFIG_UPDATE)
  default:
    return "unknown";
  }
  return "";
}

void printDetailedErrno(void)
{
  cerr << "errno = " << errno << ": ";
  string msg;
  switch(errno)
  {
  case EAGAIN:
    msg = "EAGAIN";
    break;
  case EBADF:
    msg = "EBADF";
    break;
  case EFAULT:
    msg = "EFAULT";
    break;
  case EINTR:
    msg = "EINTR";
    break;
  case EINVAL:
    msg = "EINVAL";
    break;
  case EIO:
    msg = "EIO";
    break;
  case EISDIR:
    msg = "EISDIR";
    break;
  default:
    msg = "errno missing from switch";
    break;
  }
  cerr << msg << endl;
}

ssize_t writeall(int fd, void* buf, size_t count)
{
  size_t written = 0;

  while(written < count)
  {
    errno = 0;
    ssize_t n = write(fd, (char*)buf + written, count - written);
    if(die)
      return -1;
    else if(n <= 0)
    {
      if(errno == EINTR || errno == EAGAIN) // try again
        continue;
      else if(errno == EPIPE) // can not write anymore
        return -1;
      return n;
    }
    written += (size_t)n;
  }
  return (ssize_t)written;
}

ssize_t readall(int fd, void* buf, size_t count)
{
  size_t raed = 0;

  while(raed < count)
  {
    errno = 0;
    ssize_t n = read(fd, (char*)buf + raed, count - raed);
    if(die)
    {
      cerr << "Aborting readall. die is set to 1." << endl;
      return -1;
    }
    else if(n <= 0)
    {
      cerr << "Read returned with 0/error. Errno: " << errno << endl;
      if(errno == EINTR || errno == EAGAIN)
        continue;
      return n;
    }
    raed += (size_t)n;
  }
  return (ssize_t)raed;
}

#define HEADER_SIZE (sizeof(i3_ipc_header_t))

int sendMessage(int fd, uint32_t type, char* payload)
{
  i3_ipc_header_t header;
  memcpy(header.magic, I3_IPC_MAGIC, 6);
  header.type = type;
  header.size = payload == nullptr ? 0 : (uint32_t)strlen(payload);

  ssize_t sent = 0;
  sent = writeall(fd, &header, HEADER_SIZE);
  if(sent < (ssize_t)HEADER_SIZE)
  {
    if(sent == -1)
    {
      cerr << "Could not transmit header" << endl;
      printDetailedErrno();
    }
    else
      cerr << "Could not transmit full header => " << sent << "/" << HEADER_SIZE
           << endl;
    return -1;
  }
  if(header.size != 0)
  {
    sent = writeall(fd, payload, header.size);
    if(sent < (int)header.size)
    {
      if(sent == -1)
      {
        cerr << "Could not transmit message" << endl;
        printDetailedErrno();
      }
      else
        cerr << "Could not transmit full message => " << sent << "/"
             << header.size << endl;
      return -1;
    }
  }
  return 0;
}

/**
 * Waits for message and fills the recv_buffer with its payload.
 * Returns the payload as a pointer to allocated mem. (has to be freed)
 * If the message type does not include a body, NULL is returned.
 * The type of the message is written to *type;
 * If there was an error, *type is set to I3_INVALID_TYPE
 */
char* readMessage(int fd, uint32_t* type)
{
  i3_ipc_header_t header;
  ssize_t n = 0;
  n = readall(fd, &header, HEADER_SIZE);
  *type = header.type;

  if(n < (ssize_t)HEADER_SIZE)
  {
    if(n == -1)
    {
      cerr << "Could not read header" << endl;
      printDetailedErrno();
    }
    else
      cerr << "Could not read the whole header => " << n << "/" << HEADER_SIZE
           << endl;
    *type = I3_INVALID_TYPE;
    return nullptr;
  }

  if(strncmp(header.magic, I3_IPC_MAGIC, 6) != 0)
  {
    cerr << "Invalid magic string" << endl;
    *type = I3_INVALID_TYPE;
    return nullptr;
  }

  char* payload = nullptr;

  if(header.size > 0)
  {
    payload = (char*)malloc((header.size + 1) * sizeof(char));
    n = readall(fd, payload, header.size);
    if(n < header.size)
    {
      if(n == -1)
      {
        cerr << "Could not read message" << endl;
        printDetailedErrno();
      }
      else
        cerr << "Could not read the whole message" << endl;

      *type = I3_INVALID_TYPE;
      free(payload);
      return nullptr;
    }
    payload[header.size] = '\0';
  }

  return payload;
}

typedef struct sockaddr_un SocketAddr;

int init_socket(char const* path)
{
  int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
    cerr << "Error when opening socket" << endl;
    return 1;
  }
  //(void)fcntl(sockfd, F_SETFD, FD_CLOEXEC); // WTF

  SocketAddr server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sun_family = AF_LOCAL;
  strcpy(server_address.sun_path, path);

  int err = connect(
      sockfd, (struct sockaddr*)&server_address, sizeof(server_address));
  if(err != 0)
  {
    cerr << "Connect failed" << endl;
    return 1;
  }
  return sockfd;
}

/******************************************************************************/

int test_sockets(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  char path[100];
  fgets(path, 100, stdin);
  char* p = path;
  while(*p != '\n')
    p++;
  *p = '\0';
  printf("%s\n", path);
  int fd = init_socket(path);

  char msg[] = "[\"workspace\", \"mode\"]";
  sendMessage(fd, 2, msg);

  for(int i = 0; i < 100; i++)
  {
    uint32_t type;
    char* payload = readMessage(fd, &type);
    cout << "Type " << type << ": " << payload << endl << endl;
    free(payload);
  }

  return 0;
}
