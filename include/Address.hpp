#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include <string>

#include <netdb.h>      // struct addrinfo
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h> // struct sockaddr_un

#include "utils/resource.hpp"

class Address final {
  private:
    std::string host;
    unsigned int port;

    /* struct addrinfo {
       int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
       int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
       int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
       int              ai_protocol;  // use 0 for "any"
       size_t           ai_addrlen;   // size of ai_addr in bytes
       struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
       char            *ai_canonname; // full canonical host

       struct addrinfo *ai_next;      // linked list, next node
   };*/

  public:
    Address();
    // Unix addresses don't use port numbers.
    explicit Address(std::string host, unsigned int port = 0);

    using UniqueAddrUn = UniqueResource<struct sockaddr_un>;
    UniqueAddrUn as_sockaddr_un() const;

    struct addrinfo* get_addrinfo(int family = AF_UNSPEC, int socktype = SOCK_STREAM) const;
    using UniqueAddrIn = UniqueResource<struct sockaddr_in>;
    UniqueAddrIn as_sockaddr_in() const;
    using UniqueAddrIn6 = UniqueResource<struct sockaddr_in6>;
    UniqueAddrIn6 as_sockaddr_in6() const;

    // bool run_DNS_lookup();
    // int open_TCP_socket();
    //
    // void print_sockaddr(struct sockaddr& addr);
    // void print_sockaddr_in(struct sockaddr_in& addr);
    // void print_addr(int af, void* addr);
    // void print_sockaddr_in6(struct sockaddr_in6& addr);
};

#endif
