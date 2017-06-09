#ifndef __STRUTIL_LOL___
#define __STRUTIL_LOL___

#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <stack>
#include <string>

char next(char const*& string);
void whitespace(char const*& string);
void nonspace(char const*& string);
double number(char const*& string);
std::string escaped_string(char const*& string);

bool hasInput(int fd, int microsec);

void store_string(char* dst, size_t dst_size, char* src, size_t src_size);

bool load_file(std::string const& name, std::string& content);

std::ostream&
print_time(std::ostream& out, struct tm* ptm, char const* const format);

ssize_t read_all(int fd, char* buf, size_t count, bool& die);
ssize_t write_all(int fd, char const* buf, size_t count, bool& die);

#endif
