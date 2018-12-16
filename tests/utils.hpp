#ifndef TESTS_UTILS_HPP
#define TESTS_UTILS_HPP

#include <string>
#include <utility>

#include <cstdio>

#include <unistd.h>

#include <bandit/bandit.h>
#include <fmt/format.h>

#include "utils/io.hpp"

// Create a pipe and assign the in and out sockets to the passed handles.
std::pair<UniqueSocket, UniqueSocket> make_pipe(int& in, int& out);

// Write the `data` to `in` and assert that `num` characters have been written.
void assert_write(int in, std::string const& data, size_t num);

// Read `NUM` characters from `out` and assert that these are equal to `expect`.
template <int NUM>
void assert_read(int out, std::string const& expect) {
    char buffer[NUM];
    memset(buffer, '\0', NUM);
    AssertThat(read(out, buffer, NUM), snowhouse::Equals(NUM));
    AssertThat(std::string(buffer, NUM), snowhouse::Equals(expect));
}

// Create a unique temporary file name.
std::string make_filename();

// Data type for an automatically deleted temporary file.
using TempFile = UniqueResource<std::string>;

// Create a temporary file.
TempFile make_file();

// Write `content` to the file given by `filename`.
void write_to_file(std::string const& filename, std::string const& content);

// Create a bidirectional pipe using unix sockets and assign the server and
// client sockets to the passed handles.
std::pair<UniqueSocket, UniqueSocket> make_bidirectional_pipe(int& server, int& client);

#endif
