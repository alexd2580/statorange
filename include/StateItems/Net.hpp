#ifndef STATEITEMS_NET_HPP
#define STATEITEMS_NET_HPP

#include <ostream>
#include <string>
#include <utility>
#include <chrono>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"

class Net final : public StateItem {
  public:
    enum class Type { wireless, ethernet };

    static Type parse_type(std::string const& type);

    enum class Display {
        None,         // `interface`
        IPv4,         // `ipv4`
        IPv6,         // `ipv6`
        Both          // `ipv4 | ipv6`
    };

    static Display parse_display(std::string const& display);

  private:
    std::string name;
    Type type;
    Display display;
    std::chrono::seconds connection_check_cooldown;
    std::chrono::system_clock::time_point last_connection_check;

    bool up;
    std::string ipv4;
    bool ipv4_connected;
    std::string ipv6;
    bool ipv6_connected;

    // Optional (wireless properties).
    std::string essid;
    int32_t quality;
    int32_t bitrate;

    static std::chrono::seconds min_cooldown;

    // addresses :: Map Interface (IPv4_Addr, IPv6_Addr)
    static std::map<std::string, std::pair<std::string, std::string>> addresses;
    static bool update_ip_addresses();

    std::pair<bool, bool> update_wireless_state();

    std::pair<bool, bool> update_connection_state();

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar& bar, uint8_t) override; // NOLINT

  public:
    explicit Net(JSON::Node const&);
};

#endif
