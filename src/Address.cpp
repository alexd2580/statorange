#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Address.hpp"

using namespace std;

Address::Address(std::string hostname_, unsigned int port_)
    : hostname(hostname_), port(port_)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));

  // int ai_flags; // AI_PASSIVE, AI_CANONNAME, etc.
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; // SOCK_DGRAM
  hints.ai_protocol = 0;           // use 0 for "any"
  // size_t ai_addrlen; // size of ai_addr in bytes
  // struct sockaddr* ai_addr; // struct sockaddr_in or _in6
  // char* ai_canonname; // full canonical hostname
  // struct addrinfo* ai_next; // linked list, next node

  struct addrinfo* addresses = nullptr;
  string service = std::to_string(port);
  int res = getaddrinfo(hostname.c_str(), service.c_str(), &hints, &addresses);

  if(res != 0)
  {
    cerr << gai_strerror(res);
    return;
  }

  memset(&addrinfo, 0, sizeof(struct addrinfo));
  int i = 0;
  cout << "Lookup of " << hostname << ":" << port
       << " returned the following addresses" << endl;
  for(struct addrinfo* addr_ptr = addresses; addr_ptr != nullptr;
      addr_ptr = addr_ptr->ai_next)
  {
    cout << i << ": ";
    print_sockaddr(*(addr_ptr->ai_addr));
    i++;
  }

  cout << "Choosing the first one" << endl;
  memcpy(&addrinfo, addresses, sizeof(struct addrinfo));
  addrinfo.ai_next = nullptr;

  freeaddrinfo(addresses);
}

int Address::openTCPSocket(void)
{
  int fd =
      socket(addrinfo.ai_family, addrinfo.ai_socktype, addrinfo.ai_protocol);
  if(fd == -1)
  {
    perror("Could not create socket");
    return -1;
  }

  int res = connect(fd, addrinfo.ai_addr, addrinfo.ai_addrlen);
  if(res != 0)
  {
    perror("Could not connect to server");
    return -1;
  }
  return fd;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
void Address::print_sockaddr(struct sockaddr& addr)
{
  /*struct sockaddr
  {
    unsigned short sa_family; // address family, AF_xxx
    char sa_data[14];         // 14 bytes of protocol address
  };*/
  switch(addr.sa_family)
  {
  case AF_INET:
    print_sockaddr_in(*(struct sockaddr_in*)&addr);
    break;
  case AF_INET6:
    print_sockaddr_in6(*(struct sockaddr_in6*)&addr);
    break;
  default:
    cout << "Unknown address family " << (int)addr.sa_family;
    break;
  }
}
#pragma clang diagnostic pop

void Address::print_sockaddr_in(struct sockaddr_in& addr)
{
  /*struct sockaddr_in
  {
    short int sin_family;        // Address family, AF_INET
    unsigned short int sin_port; // Port number
    struct in_addr sin_addr;     // Internet address
    unsigned char sin_zero[8];   // Same size as struct sockaddr
  };*/
  cout << "IPv4 address" << endl;
  cout << "Port: " << ntohs(addr.sin_port) << endl;
  print_addr(AF_INET, &addr.sin_addr);
}

void Address::print_addr(int af, void* addr)
{
  /*struct in_addr
  {
    uint32_t s_addr; // that's a 32-bit int (4 bytes)
  };
  struct in6_addr
  {
    unsigned char s6_addr[16]; // IPv6 address
  };*/

  char buf[100];
  const char* res = inet_ntop(af, addr, buf, 100);
  if(res == nullptr)
  {
    perror("Failed to print address");
    return;
  }
  cout << "Address: " << res << endl;
}

void Address::print_sockaddr_in6(struct sockaddr_in6& addr)
{
  /*struct sockaddr_in6
  {
    u_int16_t sin6_family;     // address family, AF_INET6
    u_int16_t sin6_port;       // port number, Network Byte Order
    u_int32_t sin6_flowinfo;   // IPv6 flow information
    struct in6_addr sin6_addr; // IPv6 address
    u_int32_t sin6_scope_id;   // Scope ID
  };*/
  cout << "IPv6 address" << endl;
  cout << "Port: " << ntohs(addr.sin6_port) << endl;
  print_addr(AF_INET6, &addr.sin6_addr);
}

char to_hex_digit(uint8_t a)
{
  if(a < 10)
    return (char)('0' + a);
  else
    return (char)('A' + a - 10);
}

void print_hex_byte(uint8_t a)
{
  cout << to_hex_digit(a / 16) << to_hex_digit(a % 16);
}

/*int main(void)
{
  Address a("localhost", 8081);
  int fd = a.openTCPSocket();
  close(fd);
}*/
