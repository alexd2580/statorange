#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <cassert>
#include <cstring>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.hpp"

// std::string const shell_file_loc = "/bin/sh";
// std::string const terminal_cmd = "x-terminal-emulator -e";

ParseException::ParseException(std::string new_message) : message(std::move(new_message)) {}
const char* ParseException::what() const noexcept { return message.c_str(); }

StringPointer::StringPointer(char const* new_base) : base(new_base), offset(0) {
    if(new_base == nullptr) {
        throw ParseException("Cannot create `StringPointer` to `nullptr`.");
    }
}

StringPointer::operator char const*() const { // NOLINT: Implicit conversion desired.
    return base + offset;                     // NOLINT: Pointer arithmetic intended.
}

char StringPointer::peek() const { return base[offset]; } // NOLINT: Pointer arithmetic intended.

void StringPointer::skip(size_t num) { offset += num; }

char StringPointer::next() {
    return base[offset++]; // NOLINT: Pointer arithmetic intended.
}

void StringPointer::whitespace() {
    char c = peek();
    while((c == ' ' || c == '\n' || c == '\t') && c != '\0') {
        c = base[++offset]; // NOLINT: Pointer arithmetic intended.
    }
}

void StringPointer::nonspace() {
    char c = peek();
    while(c != ' ' && c != '\n' && c != '\t' && c != '\0') {
        c = base[++offset]; // NOLINT: Pointer arithmetic intended.
    }
}

std::string StringPointer::escaped_string() {
    char c = peek();
    if(c != '"') {
        throw ParseException("Expected [\"], got: [" + c + "].");
    }

    std::ostringstream o;
    ssize_t start = offset;
    c = next();

    while(c != '"' && c != '\0') {
        if(c == '\\') // If an escaped char is found.
        {
            // `str` points to one character ahead.
            // Push the existing string and grab the next character.
            ssize_t len = offset - start - 1;
            o << std::string(base + start, static_cast<size_t>(len)); // NOLINT: Pointer arithmetic intended.
            c = next();

            switch(c) {
            case '\0':
                throw ParseException("Unexpected EOS when parsing escaped character.");
            case 'n':
                o << '\n';
                break;
            case 't':
                o << '\t';
                break;
            case 'r':
                o << '\r';
                break;
            default:
                std::cerr << "Invalid escape sequence: '\\" << c << "'" << std::endl;
                o << c;
                break;
            }
            start = offset;
        }
        c = next();
    }

    if(c == '\0') {
        throw ParseException("Unexpected EOS");
    }

    ssize_t len = offset - start - 1;
    o << std::string(base + start, static_cast<size_t>(len)); // NOLINT: Pointer arithmetic intended.

    return o.str();
}

bool has_input(int fd, int microsec) {
    fd_set rfds;
    FD_ZERO(&rfds);    // NOLINT: Macro use with inline assembler intended.
    FD_SET(fd, &rfds); // NOLINT: Macro use with inline assembler intended.

    struct timeval tv {};
    tv.tv_sec = 0;
    tv.tv_usec = microsec;

    return select(fd + 1, &rfds, nullptr, nullptr, &tv) == 1;
}

bool load_file(std::string const& name, std::string& content) {
    std::fstream file(name.c_str(), std::fstream::in);
    if(file.is_open()) {
        // We use the standard getline function to read the file into
        // a std::string, stoping only at "\0"
        getline(file, content, '\0');
        bool ret = !file.bad();
        file.close();
        return ret;
    }
    return false;
}

#include <chrono>  // std::chrono::system_clock
#include <ctime>   // strftime
#include <iomanip> // std::put_time

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

ssize_t write_all(int fd, char const* buf, size_t count) {
    size_t bytes_written = 0;

    while(bytes_written < count) {
        errno = 0;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        ssize_t n = write(fd, buf + bytes_written, count - bytes_written);
        if(n <= 0) {
            if(errno == EINTR || errno == EAGAIN) { // try again
                continue;
            }
            if(errno == EPIPE) { // can not write anymore
                return -1;
            }
            return n;
        }
        bytes_written += static_cast<size_t>(n);
    }
    return static_cast<ssize_t>(bytes_written);
}

ssize_t read_all(int fd, char* buf, size_t count) {
    size_t bytes_read = 0;

    while(bytes_read < count) {
        errno = 0;
        // NOLINTNEXTLINE: Pointer arithmetic intended.
        ssize_t n = read(fd, buf + bytes_read, count - bytes_read);
        if(n <= 0) {
            std::cerr << "Read returned with 0/error. Errno: " << errno << std::endl;
            if(errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return n;
        }
        bytes_read += static_cast<size_t>(n);
    }
    assert(bytes_read == count); // NOLINT: Calling `assert` triggers array-decay-lint.
    return static_cast<ssize_t>(bytes_read);
}

void make_hex(std::string::iterator dst, uint8_t a) {
    static std::string const hex_chars = "0123456789ABCDEF";
    *dst = hex_chars[a / 16];
    *(dst + 1) = hex_chars[a % 16];
}

std::string make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    std::string res{"123456789"};
    auto it = res.begin();
    *it = '#';
    make_hex(it + 1, a);
    make_hex(it + 3, r);
    make_hex(it + 5, g);
    make_hex(it + 7, b);
    return res;
}

FileStream::FileStream(int new_fd) : fd(new_fd) {}
FileStream::FileStream(FILE* f) : FileStream(fileno(f)) {}

int FileStream::underflow() {
    if(buffered_char == EOF) {
        char c;
        buffered_char = read_all(fd, &c, 1) == 1 ? static_cast<int>(c) : EOF;
    }
    return buffered_char;
}

int FileStream::uflow() {
    if(buffered_char == EOF) {
        char c;
        return read_all(fd, &c, 1) == 1 ? static_cast<int>(c) : EOF;
    }
    auto c = static_cast<char>(buffered_char);
    buffered_char = EOF;
    return c;
}

int FileStream::overflow(int i) {
    auto c = static_cast<char>(i);
    return write_all(fd, &c, 1) == 1 ? i : EOF;
}

UniqueFile open_file(std::string const& path) { return {fopen(path.c_str(), "re"), fclose}; }

#include <memory>

UniqueFile run_command(std::string const& command, std::string const& mode) {
    // NOLINTNEXTLINE: TODO: Replace by CERT-ENV33-C compliant solution.
    return {popen(command.c_str(), mode.c_str()), pclose};
}

UniqueSocket::UniqueSocket(int new_sockfd) : UniqueResource(new_sockfd, close) {}
UniqueSocket::operator int() { return resource; }
