#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "util.hpp"
#include "Logger.hpp"

class Address : public Logger
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
  Address(void);
  Address(std::string hostname, unsigned int port);
  Address& operator=(Address const& rvalue);
  virtual ~Address() {}

  bool run_DNS_lookup(void);
  int open_TCP_socket(void);

  void print_sockaddr(struct sockaddr& addr);
  void print_sockaddr_in(struct sockaddr_in& addr);
  void print_addr(int af, void* addr);
  void print_sockaddr_in6(struct sockaddr_in6& addr);
};
