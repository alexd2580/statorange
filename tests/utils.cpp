#include <fstream>
#include <future>
#include <string>
#include <utility>

#include <cstdio>

#include <arpa/inet.h> // AF_LOCAL, SOCK_STREAM
#include <fcntl.h>
#include <sys/un.h> // struct sockaddr_un
#include <unistd.h>

#include <bandit/bandit.h>
#include <fmt/format.h>

#include "utils/io.hpp"

// Local import.
#include "utils.hpp"

using namespace bandit;
using namespace snowhouse;

std::pair<UniqueSocket, UniqueSocket> make_pipe(int& in, int& out) {
    int fd[2];
    AssertThat(pipe(fd), Equals(0));
    out = fd[0];
    in = fd[1];
    return {UniqueSocket(in), UniqueSocket(out)};
}

void assert_write(int in, std::string const& data, size_t num) {
    AssertThat(write(in, data.c_str(), num), Equals(num));
}

std::string make_filename() {
    char buffer[L_tmpnam + 1];
    char* buffer_ptr = static_cast<char*>(buffer);
    memset(buffer_ptr, '\0', L_tmpnam + 1);
    AssertThat(tmpnam(buffer_ptr), Equals(buffer_ptr));
    return std::string(buffer_ptr);
}

using TempFile = UniqueResource<std::string>;

TempFile make_file() {
    auto filename = make_filename();
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_NOCTTY | O_NONBLOCK, 0666);
    close(fd);
    return TempFile(filename, [](std::string fname) { assert(unlink(fname.c_str()) == 0); });
}

void write_to_file(std::string const& filename, std::string const& content) {
    std::ofstream file;
    file.open(filename);
    file << content;
    file.close();
}

std::pair<UniqueSocket, UniqueSocket> make_bidirectional_pipe(int& server, int& client) {
    // Create server socket.
    int server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    AssertThat(server_socket, Is().Not().EqualTo(-1));

    const socklen_t sockaddr_un_size = sizeof(struct sockaddr_un);

    // Create temp file path and bind socket to it.
    auto const socket_filename = make_filename();
    struct sockaddr_un server_address {};
    memset(&server_address, 0, sockaddr_un_size);
    server_address.sun_family = AF_LOCAL;
    strncpy(static_cast<char*>(server_address.sun_path), socket_filename.c_str(), socket_filename.length());
    struct sockaddr* server_address_ptr = reinterpret_cast<struct sockaddr*>(&server_address);
    AssertThat(bind(server_socket, server_address_ptr, sockaddr_un_size), Is().Not().EqualTo(-1));
    AssertThat(listen(server_socket, 1), Is().Not().EqualTo(-1));

    auto future = std::async(std::launch::async, [&socket_filename] { return connect_to(socket_filename); });

    client = accept(server_socket, nullptr, nullptr);
    AssertThat(client, Is().Not().EqualTo(-1));
    server = future.get();
    AssertThat(server, Is().Not().EqualTo(-1));

    AssertThat(close(server_socket), Is().Not().EqualTo(-1));
    return {UniqueSocket(server), UniqueSocket(client)};
}
