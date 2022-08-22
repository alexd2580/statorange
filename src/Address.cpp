#include <string>

#include <cstring>

#include <netdb.h>      // struct addrinfo
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h> // struct sockaddr_un

#include "Address.hpp"

#include "Logger.hpp"
#include "utils/resource.hpp"

Address::Address() : Address("0.0.0.0") {}

Address::Address(std::string host_, unsigned int port_) : host(std::move(host_)), port(port_) {}

Address::UniqueAddrUn Address::as_sockaddr_un() const {
    struct sockaddr_un address {};
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_LOCAL;
    strncpy(static_cast<char*>(address.sun_path), host.c_str(), host.length());
    return UniqueAddrUn{address};
}

struct addrinfo* Address::get_addrinfo(int family, int socktype) const {
    struct addrinfo hints {};
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = 0;

    struct addrinfo* addresses = nullptr;
    std::string service = std::to_string(port);

    int res = getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses);
    if(res != 0) {
        LOG << host << ":" << service << " => " << gai_strerror(res) << std::endl;
        return nullptr;
    }

    return addresses;
}

Address::UniqueAddrIn Address::as_sockaddr_in() const {
    struct addrinfo* addresses = get_addrinfo(AF_INET);
    if(addresses == nullptr) {
        return UniqueAddrIn{};
    }
    struct sockaddr_in address {};
    memset(&address, 0, sizeof(address));
    memcpy(&address, addresses->ai_addr, sizeof(address));
    freeaddrinfo(addresses);
    return UniqueAddrIn{address};
}

Address::UniqueAddrIn6 Address::as_sockaddr_in6() const {
    struct addrinfo* addresses = get_addrinfo(AF_INET6);
    if(addresses == nullptr) {
        return UniqueAddrIn6{};
    }
    struct sockaddr_in6 address {};
    memset(&address, 0, sizeof(address));
    memcpy(&address, addresses->ai_addr, sizeof(address));
    freeaddrinfo(addresses);
    return UniqueAddrIn6{address};
}

// bool Address::run_DNS_lookup(void)
// {
//     struct addrinfo hints;
//     memset(&hints, 0, sizeof(struct addrinfo));
//
//     // int ai_flags; // AI_PASSIVE, AI_CANONNAME, etc.
//     hints.ai_family = AF_UNSPEC;
//     hints.ai_socktype = SOCK_STREAM; // SOCK_DGRAM
//     hints.ai_protocol = 0;           // use 0 for "any"
//     // size_t ai_addrlen; // size of ai_addr in bytes
//     // struct sockaddr* ai_addr; // struct sockaddr_in or _in6
//     // char* ai_canonname; // full canonical host
//     // struct addrinfo* ai_next; // linked list, next node
//
//     struct addrinfo* addresses = nullptr;
//     string service = std::to_string(port);
//
//     LOG << "Running DNS lookup of " << host << " " << service << endl;
//
//     int res =
//         getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses);
//
//     if(res != 0)
//     {
//         LOG << gai_strerror(res) << endl;
//         return false;
//     }
//
//     if(addresses == nullptr)
//     {
//         LOG << "Empty list" << endl;
//         return false;
//     }
//
//     int i = 0;
//     LOG << "Lookup of " << host << ":" << port
//           << " returned the following addresses" << endl;
//     for(struct addrinfo* addr_ptr = addresses; addr_ptr != nullptr;
//         addr_ptr = addr_ptr->ai_next)
//     {
//         LOG << i << ": " << endl;
//         print_sockaddr(*(addr_ptr->ai_addr));
//         i++;
//     }
//
//     LOG << "Choosing the first one" << endl;
//     memset(&addrinfo, 0, sizeof(struct addrinfo));
//     memcpy(&addrinfo, addresses, sizeof(struct addrinfo));
//     addrinfo.ai_next = nullptr;
//
//     freeaddrinfo(addresses);
//     return true;
// }
//
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wcast-align"
// void print_sockaddr(struct sockaddr& addr)
// {
//     /*struct sockaddr
//     {
//       unsigned short sa_family; // address family, AF_xxx
//       char sa_data[14];         // 14 bytes of protocol address
//     };*/
//     switch(addr.sa_family)
//     {
//     case AF_INET:
//         print_sockaddr_in(*(struct sockaddr_in*)&addr);
//         break;
//     case AF_INET6:
//         print_sockaddr_in6(*(struct sockaddr_in6*)&addr);
//         break;
//     default:
//         LOG << "Unknown address family " << (int)addr.sa_family << endl;
//         break;
//     }
// }
// #pragma clang diagnostic pop
//
// void Address::print_sockaddr_in(struct sockaddr_in& addr)
// {
//     /*struct sockaddr_in
//     {
//       short int sin_family;        // Address family, AF_INET
//       unsigned short int sin_port; // Port number
//       struct in_addr sin_addr;     // Internet address
//       unsigned char sin_zero[8];   // Same size as struct sockaddr
//     };*/
//     LOG << "IPv4 address" << endl;
//     LOG << "Port: " << ntohs(addr.sin_port) << endl;
//     print_addr(AF_INET, &addr.sin_addr);
// }

// void Address::print_addr(int af, void* addr)
// {
//     /*struct in_addr
//     {
//       uint32_t s_addr; // that's a 32-bit int (4 bytes)
//     };
//     struct in6_addr
//     {
//       unsigned char s6_addr[16]; // IPv6 address
//     };*/
//
//     char buf[100];
//     const char* res = inet_ntop(af, addr, buf, 100);
//     if(res == nullptr)
//     {
//         LOG << "Failed to print address: " << strerror(errno) << endl;
//         return;
//     }
//     LOG << "Address: " << res << endl;
// }
//
// void Address::print_sockaddr_in6(struct sockaddr_in6& addr)
// {
//     /*struct sockaddr_in6
//     {
//       u_int16_t sin6_family;     // address family, AF_INET6
//       u_int16_t sin6_port;       // port number, Network Byte Order
//       u_int32_t sin6_flowinfo;   // IPv6 flow information
//       struct in6_addr sin6_addr; // IPv6 address
//       u_int32_t sin6_scope_id;   // Scope ID
//     };*/
//     LOG << "IPv6 address" << endl;
//     LOG << "Port: " << ntohs(addr.sin6_port) << endl;
//     print_addr(AF_INET6, &addr.sin6_addr);
// }
