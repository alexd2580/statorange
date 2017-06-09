#ifndef __I3IPCINTERFACE__
#define __I3IPCINTERFACE__

#include <cstdint>
#include <string>
#include <unistd.h>
#include <memory>

std::string ipc_type_to_string(unsigned int type);

int init_socket(std::string const& path);

/**
 * Reads a message from the ipc socket given by fd.
 * Blocking call, die is used to abort the reentry into a blocking call.
 */
std::unique_ptr<char[]> read_message(int fd, uint32_t& type, bool& die);

/**
 * Transmits a message given by (type,payload) to the ipc socket fd.
 * Prints an error message to cerr and returns false on error.
 * die is used to abort reentry into blocking calls.
 */
bool send_message(int fd, uint32_t type, bool& die, std::string const& payload = "");

#endif
