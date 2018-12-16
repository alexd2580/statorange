#ifndef I3_IPC_HPP
#define I3_IPC_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <unistd.h>

namespace i3_ipc {
/**
 * Return a string representation of the ipc response message type.
 */
std::string type_to_string(unsigned int type);

/**
 * Transmits a message given by (type,payload) to the ipc socket fd.
 * Prints error messages to cerr and returns false on error.
 */
bool write_message(int fd, uint32_t type, std::string const& payload = "");

/**
 * Reads a message from the ipc socket given by fd. Blocking call.
 */
std::unique_ptr<char[]> read_message(int fd, uint32_t& type);

/**
 * Send a message pf type `type` to the socket `fd` and returns the response.
 */
std::unique_ptr<char[]> query(int fd, uint32_t type);
} // namespace i3_ipc

#endif
