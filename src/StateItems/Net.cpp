#include <cstdlib>
#include <cstring>
#include <ostream>

#include "StateItems/Net.hpp"

Net::Type Net::parse_type(std::string const& type) {
#define PARSE_TYPE(enum)                                                                                               \
    if(type == #enum) {                                                                                                \
        return Net::Type::enum;                                                                                        \
    }
    PARSE_TYPE(ethernet)
    PARSE_TYPE(wireless)
    throw "Couldn't parse connection type."; // NOLINT
}

Net::Display Net::parse_display(std::string const& display) {
#define PARSE_DISPLAY(enum)                                                                                            \
    if(display == #enum) {                                                                                             \
        return Net::Display::enum;                                                                                     \
    }

    PARSE_DISPLAY(None)
    PARSE_DISPLAY(IPv4)
    PARSE_DISPLAY(IPv6)
    PARSE_DISPLAY(Both)
    return Net::Display::None;
}

time_t Net::min_cooldown = 1000; // TODO magicnumber
std::map<std::string, std::pair<std::string, std::string>> Net::addresses;

/******************************************************************************/
/******************************************************************************/

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/types.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
bool Net::get_IP_addresses(Logger const& logger) {
    static time_t last_updated = 0;
    time_t now = time(nullptr);
    if(now < last_updated + min_cooldown) {
        return true;
    }

    last_updated = now;

    addresses.clear();

    struct ifaddrs* base;
    struct ifaddrs* ifa;
    void* addr_ptr;

    if(getifaddrs(&base) != 0) {
        return true;
    }
    ifa = base;

    while(ifa != nullptr) {
        if(ifa->ifa_addr != nullptr) {
            std::string iface(ifa->ifa_name); // NOLINT: TODO what function?
            auto family = ifa->ifa_addr->sa_family;
            if(family == AF_INET || family == AF_INET6) {
                auto address_emplace_result = addresses.emplace(iface, std::pair<std::string, std::string>{"", ""});
                auto& address_entry = address_emplace_result.first->second;

                if(family == AF_INET) {
                    // NOLINTNEXTLINE: C way of doing this.
                    addr_ptr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                    char buffer[INET_ADDRSTRLEN];
                    auto buffer_ptr = static_cast<char*>(buffer);
                    inet_ntop(AF_INET, addr_ptr, buffer_ptr, INET_ADDRSTRLEN);

                    address_entry.first = std::string(buffer_ptr);
                    // logger.log() << "IPv4 " << iface << ": " << address_entry.first << std::endl;
                } else if(family == AF_INET6) {
                    // NOLINTNEXTLINE: C style of doing things.
                    addr_ptr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
                    char buffer[INET6_ADDRSTRLEN];
                    auto buffer_ptr = static_cast<char*>(buffer);
                    inet_ntop(AF_INET6, addr_ptr, buffer_ptr, INET6_ADDRSTRLEN);
                    address_entry.second = std::string(buffer_ptr);
                    // logger.log() << "IPv6 " << iface << ": " << address_entry.second << std::endl;
                }
            }
        }
        ifa = ifa->ifa_next;
    }
    if(base != nullptr) {
        freeifaddrs(base);
    }
    return true;
}
#pragma clang diagnostic pop

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

std::pair<bool, bool> Net::get_wireless_state() {
    UniqueSocket sockfd(socket(AF_INET, SOCK_DGRAM, 0));
    if(sockfd == -1) {
        log() << "Cannot open socket" << std::endl;
        log_errno();
        return {false, false};
    }

    struct iwreq wreq {};
    memset(&wreq, 0, sizeof(struct iwreq));
    snprintf(wreq.ifr_name, 7, "wlp2s0"); // NOLINT: C style of doing things.

    // Essid.
    int const ESSID_LENGTH = 32;
    char buffer[ESSID_LENGTH];
    char* buffer_ptr = static_cast<char*>(buffer);
    memset(buffer_ptr, 0, ESSID_LENGTH);
    wreq.u.essid.pointer = buffer_ptr;  // NOLINT: C style of doing things.
    wreq.u.essid.length = ESSID_LENGTH; // NOLINT: C style of doing things.

    if(ioctl(sockfd, SIOCGIWESSID, &wreq) == -1) { // NOLINT: C style of doing things.
        log() << "SIOCGIWESSID ioctl failed" << std::endl;
        log_errno();
        return {false, false};
    }

    essid.assign(buffer_ptr);

    // Quality.
    iw_statistics stats{};
    wreq.u.data.pointer = &stats;               // NOLINT: C style of doing things.
    wreq.u.data.length = sizeof(iw_statistics); // NOLINT: C style of doing things.

    if(ioctl(sockfd, SIOCGIWSTATS, &wreq) == -1) { // NOLINT: C style of doing things.
        log() << "SIOCGIWSTATS ioctl failed" << std::endl;
        log_errno();
        return {false, false};
    }

    if(stats.qual.updated & IW_QUAL_DBM) { // NOLINT: C style of doing things.
#define DBM_MIN (-100)
#define DBM_MAX (-20)
        int dbm = stats.qual.level - 256;
        quality = 100 * (dbm - DBM_MIN) / (DBM_MAX - DBM_MIN);
    } else {
        log() << "Cannot read quality" << std::endl;
        quality = -100;
    }

    /*** BITRATE ***/
    if(ioctl(sockfd, SIOCGIWRATE, &wreq) == -1) { // NOLINT: C style of doing things.
        log() << "SIOCGIWRATE ioctl failed" << std::endl;
        log_errno();
        return {false, false};
    }

    bitrate = wreq.u.bitrate.value / 1000000; // NOLINT: C style of doing things.

    return {true, true};
}

Net::Net(JSON::Node const& item)
    : StateItem(item), name(item["interface"].string()), type(Net::parse_type(item["type"].string())),
      display(Net::parse_display(item["display"].string())) {
    up = false;
    quality = 0;
    bitrate = 0;

    auto this_cooldown = item["cooldown"].number<time_t>();
    min_cooldown = std::min(min_cooldown, this_cooldown);
}

std::pair<bool, bool> Net::update_raw() {
    if(!get_IP_addresses(*this)) {
        return {true, false};
    }

    auto it = addresses.find(name);
    if(it == addresses.end()) {
        const bool changed = up;
        up = false;
        return {true, changed};
    }
    up = true;

    bool changed = ipv4 != it->second.first;
    ipv4.assign(it->second.first);
    changed = changed || ipv6 != it->second.second;
    ipv6.assign(it->second.second);

    bool success = true;
    if(type == Net::Type::wireless) {
        const std::pair<bool, bool> result = get_wireless_state();
        success = success && result.first;
        changed = changed || result.second;
    }

    return {success, changed};
}

void Net::print_raw(Lemonbar& bar, uint8_t display_arg) {
    (void)display_arg;
    if(up) {
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::neutral);
        switch(type) {
        case Net::Type::ethernet:
            bar() << icon << ' ' << name;
            break;
        case Net::Type::wireless:
            bar() << icon << ' ' << essid << '(' << quality << "%%) ";
            bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::neutral);
            break;
        }

        switch(display) {
        case Net::Display::IPv4:
            [[fallthrough]];
        case Net::Display::IPv6: {
            bool ipv4_main = display == Net::Display::IPv4;
            auto const& main_address = ipv4_main ? ipv4 : ipv6;
            bool main_exists = !main_address.empty();
            auto const secondary_string = ipv4_main ? "v6" : "v4";
            bool secondary_exists = !(ipv4_main ? ipv4 : ipv6).empty();
            if(main_exists) {
                bar() << ' ' << main_address << ' ';
                if(secondary_exists) {
                    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::neutral);
                }
            }
            if(secondary_exists) {
                bar() << " (" << secondary_string << ") ";
            }
        } break;
        case Net::Display::Both:
            if(!ipv4.empty() && !ipv6.empty()) {
                bar() << ' ' << ipv4 << ' ';
                bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::neutral);
                bar() << ' ' << ipv6 << ' ';
            } else if(!ipv4.empty() || !ipv6.empty()) {
                bar() << ' ' << ipv4 << ipv6 << ' ';
            }
            break;
        case Net::Display::None:
            bar() << " Up ";
            break;
        }
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
    }
}
