#include <array>    // array
#include <fstream>  // ifstream
#include <iomanip>  // std::put_time
#include <iostream> // cerr
#include <memory>   // unique_ptr
#include <string>   // string

#include <cassert> // assert
#include <cstdio>  // popen, fopen

#include <sys/select.h> // select, FD_ZERO, FD_SET
#include <unistd.h>     // write, read

#include <fmt/format.h>

#include "utils/io.hpp"

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
