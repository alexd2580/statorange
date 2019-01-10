#include <array>    // array
#include <fstream>  // ifstream
#include <iomanip>  // std::put_time
#include <iostream> // cerr
#include <memory>   // unique_ptr
#include <string>   // string

#include <cassert> // assert
#include <cstdio>  // popen, fopen

#include <arpa/inet.h>  // AF_LOCAL, SOCK_STREAM
#include <fcntl.h>      // fcntl
#include <sys/select.h> // select, FD_ZERO, FD_SET
#include <sys/un.h>     // struct sockaddr_un
#include <unistd.h>     // write, read

#include <fmt/format.h>

#include "Address.hpp"
#include "Logger.hpp"
#include "utils/io.hpp"
#include "utils/resource.hpp"

bool has_input(int fd, int microsec) {
    fd_set rfds;
    FD_ZERO(&rfds);    // NOLINT: Macro use with inline assembler intended.
    FD_SET(fd, &rfds); // NOLINT: Macro use with inline assembler intended.

    struct timeval tv {};
    tv.tv_sec = 0;
    tv.tv_usec = microsec;

    return select(fd + 1, &rfds, nullptr, nullptr, &tv) == 1;
}

ssize_t read_all(int fd, char* buf, size_t count) {
    size_t bytes_read = 0;
    while(bytes_read < count) {
        errno = 0;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        ssize_t n = read(fd, buf + bytes_read, count - bytes_read);
        if(n > 0) {
            bytes_read += static_cast<size_t>(n);
        } else if(n == 0) {
            // EOS has been reached.
            close(fd);
            break;
        } else if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            // Ignore `EINTR`, `EAGAIN` or `EWOULDBLOCK` and retry.
            // TODO This might result in a busy wait on nonblocking sockets.
        } else {
            // Otherwise there was a critical non-recoverable error.
            throw StreamException(fmt::format("Read returned {} with errno set to {}", n, errno));
        }
    }
    return static_cast<ssize_t>(bytes_read);
}

ssize_t write_all(int fd, char const* buf, size_t count) {
    size_t bytes_written = 0;
    while(bytes_written < count) {
        errno = 0;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        ssize_t n = write(fd, buf + bytes_written, count - bytes_written);
        // Check if pipe is still open and more data can be written.
        if(n >= 0) {
            bytes_written += static_cast<size_t>(n);
        } else if(errno == EPIPE) {
            // Remote has closed the connection.
            close(fd);
            break;
        } else if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            // Ignore `EINTR`, `EAGAIN` or `EWOULDBLOCK` and retry.
            // TODO This might result in a busy wait on nonblocking sockets.
        } else { // can not write anymore
            // Otherwise there was a critical non-recoverable error.
            throw StreamException(fmt::format("Write returned {} with errno set to {}", n, errno));
        }
    }
    return static_cast<ssize_t>(bytes_written);
}

UniqueSocket::UniqueSocket() {}
UniqueSocket::UniqueSocket(int new_sockfd) : UniqueResource(new_sockfd, close) {}
UniqueSocket::operator int() const { return resource; }

bool load_file(std::string const& name, std::string& content) {
    std::ifstream file(name.c_str());
    if(file.is_open()) {
        // We use the standard getline function to read the file into
        // a std::string, stopping only at "\0"
        getline(file, content, '\0');
        bool ret = !file.fail();
        file.close();
        return ret;
    }
    return false;
}

UniqueFile run_command(std::string const& command, std::string const& mode) {
    // NOLINTNEXTLINE: TODO: Replace by CERT-ENV33-C compliant solution.
    return {popen(command.c_str(), mode.c_str()), pclose};
}

UniqueFile open_file(std::string const& path) { return {fopen(path.c_str(), "re"), fclose}; }

std::ostream& print_time(std::ostream& out, struct tm& ptm, char const* const format) {
#ifndef __GLIBCXX__
#define __GLIBCXX__ 0
#endif
#if __GLIBCXX__ >= 20151205
    return out << std::put_time(&ptm, format);
#else
    char str[256];
    auto str_ptr = static_cast<char*>(str);
    return out << (strftime(str_ptr, 256, format, &ptm) != 0 ? str_ptr : "???");
#endif
}

std::ostream& print_used_memory(std::ostream& out, uint64_t used, uint64_t total) {
    constexpr uint64_t cutoff = 1200;
    uint8_t unit_index = 0;
    uint64_t unit_factor = 1;
    while(total > cutoff * unit_factor) {
        unit_index++;
        unit_factor *= 1024;
    }
    std::array<std::string, 6> const units{"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    return out << (used / unit_factor) << '.' << ((used % unit_factor) / (unit_factor / 10)) << '/'
               << (total / unit_factor) << '.' << ((total % unit_factor) / (unit_factor / 10)) << units.at(unit_index);
}

// bind(s, (sockaddr*)&localaddr, sizeof(localaddr));
//
// sockaddr_in remoteaddr = {0};
// remoteaddr.sin_family = AF_INET;
// remoteaddr.sin_addr.s_addr = inet_addr("192.168.1.4");
// remoteaddr.sin_port = 12345; // whatever the server is listening on
// connect(s, (sockaddr*)&remoteaddr, sizeof(remoteaddr));

UniqueSocket connect_to(std::string const& path, unsigned int port, int domain, std::string const& interface) {
    return connect_to(Address(path, port), domain, interface);
}

UniqueSocket connect_to(Address const& target, int domain, std::string const& interface) {
    if(domain != AF_LOCAL && domain != AF_INET && domain != AF_INET6) {
        LOG << "Invalid domain: " << domain << std::endl;
        return UniqueSocket();
    }

    auto unique_socket = UniqueSocket(socket(domain, SOCK_STREAM, 0));
    if(unique_socket < 0) {
        LOG << "Error when opening socket." << std::endl;
        LOG_ERRNO;
        return UniqueSocket();
    }

    // int prev_flags = fcntl(sockfd, F_GETFL);
    // fcntl(sockfd, F_SETFL, prev_flags | O_NONBLOCK);

    if(!interface.empty()) {
        int result;
        Address iface_address(interface);
        switch(domain) {
        case AF_LOCAL: {
            auto address = iface_address.as_sockaddr_un();
            if(!address.is_active()) {
                LOG << "Cannot bind to socket. No such address." << std::endl;
                return UniqueSocket{};
            }
            auto address_ptr = reinterpret_cast<struct sockaddr const*>(&address.get());
            result = bind(unique_socket, address_ptr, sizeof(address));
            break;
        }
        case AF_INET: {
            auto address = iface_address.as_sockaddr_in();
            if(!address.is_active()) {
                LOG << "Cannot bind to socket. No such address." << std::endl;
                return UniqueSocket{};
            }
            // std::cout << address.sin_addr.s_addr << std::endl;
            // std::cout << address.sin_port << std::endl;
            auto address_ptr = reinterpret_cast<struct sockaddr const*>(&address.get());
            result = bind(unique_socket, address_ptr, sizeof(address));
            break;
        }
        case AF_INET6: {
            auto address = iface_address.as_sockaddr_in6();
            if(!address.is_active()) {
                LOG << "Cannot bind to socket. No such address." << std::endl;
                return UniqueSocket{};
            }

            // Print an IPv6 address.
            // char buf[100];
            // const char* res = inet_ntop(AF_INET6, &address.sin6_addr, buf, 100);
            // std::cout << "[" << res << "]" << std::endl;
            // std::cout << ntohs(address.sin6_port) << std::endl;
            // std::cout << address.sin6_scope_id << std::endl;

            auto address_ptr = reinterpret_cast<struct sockaddr const*>(&address.get());
            result = bind(unique_socket, address_ptr, sizeof(address));
            break;
        }
        default:
            assert(false);
        }
        if(result < 0) {
            LOG << "`bind` failed" << std::endl;
            LOG_ERRNO;
            return UniqueSocket();
        }
    }

    int result;
    switch(domain) {
    case AF_LOCAL: {
        auto address = target.as_sockaddr_un();
        auto address_ptr = reinterpret_cast<struct sockaddr const*>(&address.get());
        result = connect(unique_socket, address_ptr, sizeof(struct sockaddr_un));
        break;
    }
    case AF_INET: {
        auto address = target.as_sockaddr_in();
        auto address_ptr = reinterpret_cast<struct sockaddr const*>(&address.get());
        result = connect(unique_socket, address_ptr, sizeof(struct sockaddr_in));
        break;
    }
    case AF_INET6: {
        auto address = target.as_sockaddr_in6();
        auto address_ptr = reinterpret_cast<struct sockaddr const*>(&address.get());
        result = connect(unique_socket, address_ptr, sizeof(struct sockaddr_in6));
        break;
    }
    default:
        assert(false);
    }
    if(result < 0) {
        LOG << "`connect` failed" << std::endl;
        LOG_ERRNO;
        return UniqueSocket();
    }

    return unique_socket;
}
