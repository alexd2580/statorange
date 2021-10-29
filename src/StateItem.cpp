#include <climits>
#include <iostream>
#include <memory>
#include <sstream>

#include <fmt/format.h>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"

std::chrono::seconds StateItem::min_cooldown(std::numeric_limits<int>::max());

std::map<int, StateItem*> StateItem::event_sockets;

StateItem::StateItem(JSON::Node const& item)
    : Logger(item["item"].string()), module_name(item["item"].string()),
      cooldown(std::chrono::seconds(item["cooldown"].number<uint32_t>())),
      button(item["button"].exists() && item["button"].string().length() > 0), button_command(item["button"].string()),
      show_failed(item["show_failed"].boolean(true)), icon(item["icon"].string("")) {
    min_cooldown = min(min_cooldown, cooldown);
    last_updated = std::chrono::system_clock::time_point::min();

    valid = false;
}

void StateItem::register_event_socket(int fd) {
    if(event_sockets.find(fd) != event_sockets.end()) {
        log() << "Can't register event socket, socket already registered." << std::endl;
        return;
    }
    event_sockets[fd] = this;
}

void StateItem::unregister_event_socket(int fd) const { event_sockets.erase(fd); }

std::pair<bool, bool> StateItem::handle_stream_data_raw(int) {
    log() << "No `handle_events` defined for `" << module_name << "`" << std::endl;
    return {false, false};
}

bool StateItem::handle_stream_data(int fd) {
    const std::pair<bool, bool> state = handle_stream_data_raw(fd);
    changed = changed || state.first != valid || state.second;
    valid = state.first;
    return changed;
}

bool StateItem::update(bool force) {
    auto now = std::chrono::system_clock::now();
    if(now > last_updated + cooldown || force) {
        const std::pair<bool, bool> state = update_raw();
        changed = changed || state.first != valid || state.second;
        valid = state.first;
        // Make invalid items retry in the next iteration.
        if(valid) {
            last_updated = now;
        }
    }
    return changed;
}

void StateItem::print(Lemonbar& bar, uint8_t display_number) {
    if(valid) {
        if(button) {
            bar.button_begin(button_command);
        }
        print_raw(bar, display_number);
        if(button) {
            bar.button_end();
        }
    } else if(show_failed) {
        bar.separator(Lemonbar::Separator::vertical, Lemonbar::Coloring::warn);
        bar() << fmt::format(" RIP {} ", module_name);
        bar.separator(Lemonbar::Separator::vertical, Lemonbar::Coloring::white_on_black);
    }
    changed = false;
}

bool StateItem::wait_for_events(int signal_fd) {
    fd_set read_fds;
    struct timeval tv {};

    int max_fd = 0;
    FD_ZERO(&read_fds);
    if(signal_fd != 0) {
        FD_SET(signal_fd, &read_fds);
        max_fd = signal_fd;
    }
    for(auto const& event_socket : event_sockets) {
        int fd = event_socket.first;
        FD_SET(fd, &read_fds);
        max_fd = std::max(max_fd, fd);
    }

    tv.tv_sec = StateItem::min_cooldown.count();
    tv.tv_usec = 0;
    // Logger::log("StateItem") << "Setting socket timeout to " << (int)tv.tv_sec << " seconds" << std::endl;

    bool changed = false;
    int select_res = select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
    switch(select_res) {
    case -1:
        perror("select");
        break;
    case 0:
        // Timeout exceeded
        // Logger::log("StateItem") << "Timeout" << std::endl;
        break;
    default:
        // Logger::log("StateItem") << (int)select_res << " sockets are ready for reading" << std::endl;
        for(auto const& event_socket : event_sockets) {
            if(FD_ISSET(event_socket.first, &read_fds)) {
                auto item = event_socket.second;
                // Logger::log("StateItem") << "Module " << item->module_name << " can process events on socket "
                // << event_socket.first << std::endl;
                changed = changed || item->handle_stream_data(event_socket.first);
            }
        }
        break;
    }

    return changed;
}
