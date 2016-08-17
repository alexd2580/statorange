
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <csignal>
#include <sys/un.h>
#include <unistd.h>

#include "i3-ipc-constants.hpp"
#include "i3-ipc.hpp"

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
}

ssize_t write_all(int fd, char const* buf, size_t count, bool& die)
{
  size_t written = 0;

  while(written < count)
  {
    errno = 0;
    ssize_t n = write(fd, buf + written, count - written);
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

ssize_t read_all(int fd, char* buf, size_t count, bool& die)
{
  size_t raed = 0;

  while(raed < count)
  {
    errno = 0;
    ssize_t n = read(fd, buf + raed, count - raed);
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

auto kofn = [](auto k, auto n) {
  return string(std::to_string(k) + "/" + std::to_string(n));
};

bool send_message(int fd, uint32_t type, bool& die, string&& payload)
{
  // Prepare header
  i3_ipc_header_t header;
  memcpy(header.magic, I3_IPC_MAGIC, 6);
  header.type = type;
  header.size = (uint32_t)payload.length();

  // Write Header
  ssize_t sent = 0;
  sent = write_all(fd, (char*)&header, HEADER_SIZE, die);
  if(sent < (ssize_t)HEADER_SIZE)
  {
    if(sent == -1)
      perror("Could not transmit header");
    else
    {
      string msg("Could not transmit header " + kofn(sent, HEADER_SIZE));
      perror(msg.c_str());
    }
    return false;
  }

  // Write payload if present
  if(header.size != 0)
  {
    sent = write_all(fd, payload.c_str(), header.size, die);
    if(sent < (int)header.size)
    {
      if(sent == -1)
        perror("Could not transmit message");
      else
      {
        string msg("Could not transmit message " + kofn(sent, header.size));
        perror(msg.c_str());
      }
      return false;
    }
  }
  return true;
}

/**
 * Waits for message and fills the recv_buffer with its payload.
 * Returns the payload as a pointer to allocated mem. (has to be freed)
 * If the message type does not include a body, NULL is returned.
 * The type of the message is written to *type;
 * If there was an error, *type is set to I3_INVALID_TYPE
 */
std::unique_ptr<char[]> read_message(int fd, uint32_t* type, bool& die)
{
  i3_ipc_header_t header;
  ssize_t n = 0;
  n = read_all(fd, (char*)&header, HEADER_SIZE, die);
  *type = header.type;

  if(n < (ssize_t)HEADER_SIZE)
  {
    if(n == -1)
      perror("Could not read header");
    else
    {
      string msg("Could not read header " + kofn(n, HEADER_SIZE));
      perror(msg.c_str());
    }
    *type = I3_INVALID_TYPE;
    return std::unique_ptr<char[]>();
  }

  if(strncmp(header.magic, I3_IPC_MAGIC, 6) != 0)
  {
    cerr << "Invalid magic string" << endl;
    *type = I3_INVALID_TYPE;
    return std::unique_ptr<char[]>();
  }

  char* payload = nullptr;

  if(header.size > 0)
  {
    payload = new char[header.size + 1]; // why +1?
    n = read_all(fd, payload, header.size, die);
    if(n < header.size)
    {
      if(n == -1)
        perror("Could not read message");
      else
      {
        string msg("Could not read message " + kofn(n, header.size));
        perror(msg.c_str());
      }
      *type = I3_INVALID_TYPE;
      free(payload);
      return std::unique_ptr<char[]>();
    }
    payload[header.size] = '\0';
  }

  return std::unique_ptr<char[]>(payload);
}

typedef struct sockaddr_un SocketAddr;

int init_socket(string const& path)
{
  int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
    perror("Error when opening socket");
    return 1;
  }
  //(void)fcntl(sockfd, F_SETFD, FD_CLOEXEC); // WTF

  SocketAddr server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sun_family = AF_LOCAL;
  strcpy(server_address.sun_path, path.c_str());

  int err = connect(
      sockfd, (struct sockaddr*)&server_address, sizeof(server_address));
  if(err != 0)
  {
    perror("Connect failed");
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
  if(fgets(path, 100, stdin) != path)
    return 1;
  char* p = path;
  while(*p != '\n')
    p++;
  *p = '\0';
  printf("%s\n", path);
  int fd = init_socket(path);

  auto msg = string("[\"workspace\", \"mode\"]");
  bool yay = false;
  send_message(fd, 2, yay, move(msg));

  for(int i = 0; i < 100; i++)
  {
    uint32_t type;
    auto payload = read_message(fd, &type, yay);
    cout << "Type " << type << ": " << payload.get() << endl << endl;
  }

  return 0;
}
