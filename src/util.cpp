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

using namespace std;

string const shell_file_loc = "/bin/sh";
string const terminal_cmd = "x-terminal-emulator -e";

char next(char const*& string) {
    string++;
    return *(string - 1);
}

void whitespace(char const*& string) {
    char c = *string;
    while((c == ' ' || c == '\n' || c == '\t') && c != '\0') {
        string++;
        c = *string;
    }
}

void nonspace(char const*& string) {
    char c = *string;
    while(c != ' ' && c != '\n' && c != '\t' && c != '\0') {
        string++;
        c = *string;
    }
}

double number(char const*& string) {
    char* endptr;
    double n = strtod(string, &endptr);
    if(endptr == string) {
        cerr << "Could not convert string to number" << endl;
        exit(1);
    }
    string = endptr;
    return n;
}

std::string number_str(char const*& str) {
    char* endptr;
    strtod(str, &endptr);
    char const* start = str;
    str = endptr;
    return string(start, (size_t)(endptr - start));
}

string escaped_string(char const*& string) {
    char c = next(string);
    if(c != '"') {
        cerr << "Expected [\"], got: [" << c << "]." << endl;
        exit(1);
    }

    ostringstream o;
    char const* start = string;
    c = next(string);

    while(c != '"' && c != '\0') {
        if(c == '\\') // if an escaped char is found
        {
            // `string` points to one character ahead.
            size_t len = (size_t)(string - start - 1);
            o << std::string(start, len);
            c = next(string);

            switch(c) {
            case '\0':
                cerr << "Unexpected EOS when parsing escaped character." << endl;
                exit(1);
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
                cerr << "Invalid escape sequence: '\\" << c << "'" << endl;
                o << c;
                break;
            }
            start = string;
        }
        c = next(string);
    }

    if(c == '\0') {
        cerr << "Unexpected EOS" << endl;
        exit(1);
    }

    size_t len = (size_t)(string - start - 1);
    o << std::string(start, len);

    return o.str();
}

bool has_input(int fd, int microsec) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = microsec;

    return select(fd + 1, &rfds, nullptr, nullptr, &tv) == 1;
}

bool load_file(string const& name, string& content) {
    fstream file(name.c_str(), fstream::in);
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

ostream& print_time(ostream& out, struct tm& ptm, char const* const format) {
#ifndef __GLIBCXX__
#define __GLIBCXX__ 0
#endif
#if __GLIBCXX__ >= 20151205
    return out << put_time(&ptm, format);
#else
    char str[256];
    return out << (strftime(str, 256, format, &ptm) != 0) ? str : "???";
#endif
}

ssize_t write_all(int fd, char const* buf, size_t count) {
    size_t bytes_written = 0;

    while(bytes_written < count) {
        errno = 0;
        ssize_t n = write(fd, buf + bytes_written, count - bytes_written);
        if(n <= 0) {
            if(errno == EINTR || errno == EAGAIN) // try again
                continue;
            else if(errno == EPIPE) // can not write anymore
                return -1;
            return n;
        }
        bytes_written += (size_t)n;
    }
    return (ssize_t)bytes_written;
}

ssize_t read_all(int fd, char* buf, size_t count) {
    size_t bytes_read = 0;

    while(bytes_read < count) {
        errno = 0;
        ssize_t n = read(fd, buf + bytes_read, count - bytes_read);
        if(n <= 0) {
            // TODO use the logger here.
            cerr << "Read returned with 0/error. Errno: " << errno << endl;
            if(errno == EINTR || errno == EAGAIN)
                continue;
            return n;
        }
        bytes_read += (size_t)n;
    }
    assert(bytes_read == count);
    return (ssize_t)bytes_read;
}

static std::string const hex_chars = "0123456789ABCDEF";

void make_hex(std::string::iterator dst, uint8_t a) {
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

FileStream::FileStream(int fd) : fd(fd) {}
FileStream::FileStream(FILE* f) : FileStream(fileno(f)) {}

int FileStream::underflow() {
    if (buffered_char == EOF) {
        char c;
        buffered_char = read_all(fd, &c, 1) == 1 ? (int)c : EOF;
    }
    return buffered_char;
}

int FileStream::uflow() {
    if (buffered_char == EOF) {
        char c;
        return read_all(fd, &c, 1) == 1 ? (int)c : EOF;
    }
    char c = buffered_char;
    buffered_char = EOF;
    return c;
}

int FileStream::overflow(int i) {
    char c = (char)i;
    return write_all(fd, &c, 1) == 1 ? i : EOF;
}

#include<memory>

std::unique_ptr<FILE, int(*)(FILE*)> run_command(std::string const& command, std::string const& mode) {
    return std::unique_ptr<FILE, int(*)(FILE*)>(popen(command.c_str(), mode.c_str()), pclose);
}
