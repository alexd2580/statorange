#ifndef I3_IPC_CONSTANTS_HPP
#define I3_IPC_CONSTANTS_HPP

/*
 * vim:ts=4:sw=4:expandtab
 *
 * i3 - an improved dynamic tiling window manager
 * © 2009 Michael Stapelberg and contributors (see also: LICENSE)
 *
 * This public header defines the different constants and message types to use
 * for the IPC interface to i3 (see docs/ipc for more information).
 *
 * This file has been modified for use by `statorange`.
 */

#include <string>

#include <climits>
#include <cstdint>

namespace i3_ipc {
/** Never change this, only on major IPC breakage (don’t do that) */
std::string const MAGIC("i3-ipc");

struct header {
    /* 6 = strlen(MAGIC) */
    char magic[6];
    uint32_t size;
    uint32_t type;
} __attribute__((packed));
using header_t = struct header;

#define HEADER_SIZE (sizeof(header_t))

// Messages from clients to i3
namespace message_type {
// The payload of the message will be interpreted as a command.
uint32_t const COMMAND = 0;
// Requests the current workspaces from i3.
uint32_t const GET_WORKSPACES = 1;
// Subscribe to the specified events.
uint32_t const SUBSCRIBE = 2;
// Requests the current outputs from i3.
uint32_t const GET_OUTPUTS = 3;
// Requests the tree layout from i3.
uint32_t const GET_TREE = 4;
// Request the current defined marks from i3.
uint32_t const GET_MARKS = 5;
// Request the configuration for a specific 'bar'.
uint32_t const GET_BAR_CONFIG = 6;
// Request the i3 version.
uint32_t const GET_VERSION = 7;
} // namespace message_type

// Messages from i3 to clients
namespace reply_type {
// Command reply type.
uint32_t const COMMAND = 0;
// Workspaces reply type.
uint32_t const WORKSPACES = 1;
// Subscription reply type.
uint32_t const SUBSCRIBE = 2;
// Outputs reply type.
uint32_t const OUTPUTS = 3;
// Tree reply type.
uint32_t const TREE = 4;
// Marks reply type.
uint32_t const MARKS = 5;
// Bar config reply type.
uint32_t const BAR_CONFIG = 6;
// i3 version reply type.
uint32_t const VERSION = 7;
} // namespace reply_type

// Events from i3 to clients. Events have the first bit set high.
namespace event_type {
uint32_t const MASK = 1ul << 31ul;
// The workspace event will be triggered upon changes in the workspace list.
uint32_t const WORKSPACE = MASK | 0ul;
// The output event will be triggered upon changes in the output list.
uint32_t const OUTPUT = MASK | 1ul;
// The output event will be triggered upon mode changes.
uint32_t const MODE = MASK | 2ul;
// The window event will be triggered upon window changes.
uint32_t const WINDOW = MASK | 3ul;
// Bar config update will be triggered to update the bar config.
uint32_t const BARCONFIG_UPDATE = MASK | 4ul;
// The binding event will be triggered when bindings run.
uint32_t const BINDING = MASK | 5ul;
} // namespace event_type

// The type used to indicate an invalid header.
uint32_t const INVALID_TYPE = UINT_MAX;
} // namespace i3_ipc

#endif
