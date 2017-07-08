#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.hpp"

using namespace std;

string const shell_file_loc = "/bin/sh";
string const terminal_cmd = "x-terminal-emulator -e";

char next(char const*& string)
{
    string++;
    return *(string - 1);
}

void whitespace(char const*& string)
{
    char c = *string;
    while((c == ' ' || c == '\n' || c == '\t') && c != '\0')
    {
        string++;
        c = *string;
    }
}

void nonspace(char const*& string)
{
    char c = *string;
    while(c != ' ' && c != '\n' && c != '\t' && c != '\0')
    {
        string++;
        c = *string;
    }
}

double number(char const*& string)
{
    char* endptr;
    double n = strtod(string, &endptr);
    if(endptr == string)
    {
        cerr << "Could not convert string to number" << endl;
        exit(1);
    }
    string = endptr;
    return n;
}

std::string number_str(char const*& str)
{
    char* endptr;
    strtod(str, &endptr);
    char const* start = str;
    str = endptr;
    return string(start, (size_t)(endptr - start));
}

string escaped_string(char const*& string)
{
    char c = next(string);
    if(c != '"')
    {
        cerr << "Expected [\"], got: [" << c << "]." << endl;
        exit(1);
    }

    ostringstream o;
    char const* start = string;
    c = next(string);

    while(c != '"' && c != '\0')
    {
        if(c == '\\') // if an escaped char is found
        {
            // `string` points to one character ahead.
            size_t len = (size_t)(string - start - 1);
            o << std::string(start, len);
            c = next(string);

            switch(c)
            {
            case '\0':
                cerr << "Unexpected EOS when parsing escaped character."
                     << endl;
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

    if(c == '\0')
    {
        cerr << "Unexpected EOS" << endl;
        exit(1);
    }

    size_t len = (size_t)(string - start - 1);
    o << std::string(start, len);

    return o.str();
}

bool hasInput(int fd, int microsec)
{
    int retval;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = microsec;

    retval = select(fd + 1, &rfds, nullptr, nullptr, &tv);

    return retval > 0;
}

bool load_file(string const& name, string& content)
{
    fstream file(name.c_str(), fstream::in);
    if(file.is_open())
    {
        // We use the standard getline function to read the file into
        // a std::string, stoping only at "\0"
        getline(file, content, '\0');
        bool ret = !file.bad();
        file.close();
        return ret;
    }

    return false;
}

#include <chrono> // std::chrono::system_clock
#include <iomanip> // std::put_time

ostream& print_time(ostream& out, struct tm* ptm, char const* const format)
{
#ifndef __GLIBCXX__
#define __GLIBCXX__ 0
#endif
#if __GLIBCXX__ >= 20151204
    return out << put_time(ptm, format);
#else
    char str[256];
    if(strftime(str, 256, format, ptm))
        return out << str;
    else
        return out << "???";
#endif
}

ssize_t write_all(int fd, char const* buf, size_t count)
{
    size_t bytes_written = 0;

    while(bytes_written < count)
    {
        errno = 0;
        ssize_t n = write(fd, buf + bytes_written, count - bytes_written);
        if(n <= 0)
        {
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

ssize_t read_all(int fd, char* buf, size_t count)
{
    size_t bytes_read = 0;

    while(bytes_read < count)
    {
        errno = 0;
        ssize_t n = read(fd, buf + bytes_read, count - bytes_read);
        if(n <= 0)
        {
            cerr << "Read returned with 0/error. Errno: " << errno << endl;
            if(errno == EINTR || errno == EAGAIN)
                continue;
            return n;
        }
        bytes_read += (size_t)n;
    }
    return (ssize_t)bytes_read;
}
