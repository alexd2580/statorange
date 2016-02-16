#include <string>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

class Address
{

private:
  std::string hostname;
  unsigned int port;

  struct addrinfo addrinfo; /* {
     int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
     int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
     int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
     int              ai_protocol;  // use 0 for "any"
     size_t           ai_addrlen;   // size of ai_addr in bytes
     struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
     char            *ai_canonname; // full canonical hostname

     struct addrinfo *ai_next;      // linked list, next node
 };*/

public:
  Address(std::string hostname, unsigned int port);
  virtual ~Address() {}

  int openTCPSocket(void);

  static void print_sockaddr(struct sockaddr& addr);
  static void print_sockaddr_in(struct sockaddr_in& addr);
  static void print_addr(int af, void* addr);
  static void print_sockaddr_in6(struct sockaddr_in6& addr);
};
