#include <string>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <fmt/format.h>

#include "i3/ipc.hpp"
#include "i3/ipc_constants.hpp"
#include "utils/io.hpp"

namespace i3_ipc {
std::string type_to_string(uint32_t type) {
#define CASE_RETURN(l)                                                                                                 \
    case l:                                                                                                            \
        return #l;
    switch(type) {
        // The requests are not listed in this `case` because their IDs match
        // those of the responses.
        /* CASE_RETURN(message_type::COMMAND) */
        /* CASE_RETURN(message_type::GET_WORKSPACES) */
        /* CASE_RETURN(message_type::SUBSCRIBE) */
        /* CASE_RETURN(message_type::GET_OUTPUTS) */
        /* CASE_RETURN(message_type::GET_TREE) */
        /* CASE_RETURN(message_type::GET_MARKS) */
        /* CASE_RETURN(message_type::GET_BAR_CONFIG) */
        /* CASE_RETURN(message_type::GET_VERSION) */
        CASE_RETURN(reply_type::COMMAND)
        CASE_RETURN(reply_type::WORKSPACES)
        CASE_RETURN(reply_type::SUBSCRIBE)
        CASE_RETURN(reply_type::OUTPUTS)
        CASE_RETURN(reply_type::TREE)
        CASE_RETURN(reply_type::MARKS)
        CASE_RETURN(reply_type::BAR_CONFIG)
        CASE_RETURN(reply_type::VERSION)
        CASE_RETURN(event_type::WORKSPACE)
        CASE_RETURN(event_type::OUTPUT)
        CASE_RETURN(event_type::MODE)
        CASE_RETURN(event_type::WINDOW)
        CASE_RETURN(event_type::BARCONFIG_UPDATE)
        CASE_RETURN(event_type::BINDING)
        CASE_RETURN(INVALID_TYPE)
    default:
        return "unknown";
    }
}

bool write_object(int fd, char const* data, size_t size, std::string const& description) {
    ssize_t sent = write_all(fd, data, size);
    if(sent < static_cast<ssize_t>(size)) {
        std::string msg(fmt::format("Could not transmit {} {}/{}.", description, sent, size));
        perror(msg.c_str());
        return false;
    }
    return true;
}

bool write_message(int fd, uint32_t type, std::string const& payload) {
    // Prepare header.
    header_t header;
    memcpy(header.magic, MAGIC.c_str(), 6);
    header.type = type;
    header.size = (uint32_t)payload.length();

    // Write Header.
    bool success;
    success = write_object(fd, reinterpret_cast<char const*>(&header), HEADER_SIZE, "header");
    if(!success) {
        return false;
    }

    // Write payload if present.
    if(header.size == 0) {
        return true;
    }
    success = write_object(fd, payload.c_str(), header.size, "payload");
    if(!success) {
        return false;
    }

    return true;
}

bool read_object(int fd, char* buffer, size_t size, std::string const& description) {
    ssize_t n = read_all(fd, buffer, size);
    if(n < static_cast<ssize_t>(size)) {
        std::string msg(fmt::format("Could not read {} {}/{}.", description, n, HEADER_SIZE));
        perror(msg.c_str());
        return false;
    }
    return true;
}

/**
 * Waits for message and fills the recv_buffer with its payload.
 * Returns the payload as a pointer to allocated mem. (has to be freed)
 * If the message type does not include a body, nullptr is returned.
 * If there was an error, *type is set to `i3_ipc::INVALID_TYPE`.
 */
std::unique_ptr<char[]> read_message(int fd, uint32_t& type) {
    type = INVALID_TYPE;

    header_t header;
    bool success;
    success = read_object(fd, reinterpret_cast<char*>(&header), HEADER_SIZE, "header");
    if(!success) {
        return {};
    }
    if(strncmp(static_cast<char*>(header.magic), MAGIC.c_str(), 6) != 0) {
        std::cerr << "Invalid magic string" << std::endl;
        return {};
    }

    std::unique_ptr<char[]> payload;
    if(header.size != 0) {
        // To make sure that there is a `\0` byte at the end.
        payload.reset(new char[header.size + 1]);
        success = read_object(fd, payload.get(), header.size, "message");
        if(!success) {
            return {};
        }
        payload[header.size] = '\0';
    }

    type = header.type;
    return payload;
}

std::unique_ptr<char[]> query(int fd, uint32_t type) {
    bool success = write_message(fd, type);
    if(!success) {
        return {};
    }
    uint32_t ret_type = INVALID_TYPE;
    auto buffer = read_message(fd, ret_type);
    if(ret_type == INVALID_TYPE) {
        return {};
    }
    return buffer;
}
} // namespace i3_ipc
