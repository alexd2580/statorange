#ifndef STATEITEMS_NET_HPP
#define STATEITEMS_NET_HPP

#include <ostream>
#include <string>
#include <utility>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"
#include "util.hpp"

class Net final : public StateItem {
  public:
    enum class Type { wireless, ethernet };

    static Type parse_type(std::string const& type);

    enum class Display {
        none,          // shows that the interface is active
        IPv4,          // shows the ipv4 address if enabled
        IPv6,          // shows the ipv6 address if enabled
        IPv6_fallback, // shows the ipv6 address if enabled, otherwise -> ipv4
        both           // shows both addresses
    };

    static Display parse_display(std::string const& display);

  private:
    std::string name;
    Type type;
    Display display;

    bool up;
    std::string ipv4;
    std::string ipv6;

    // Optional (wireless properties).
    std::string essid;
    int32_t quality;
    int32_t bitrate;

    static time_t min_cooldown;

    // addresses :: Map Interface (IPv4_Addr, IPv6_Addr)
    static std::map<std::string, std::pair<std::string, std::string>> addresses;
    static bool get_IP_addresses(Logger const& logger);

    std::pair<bool, bool> get_wireless_state();

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar& bar, uint8_t) override; // NOLINT

  public:
    explicit Net(JSON::Node const&);
};

#endif