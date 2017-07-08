#ifndef __NETXYZHEADER_LOL___
#define __NETXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <utility>

#include "../StateItem.hpp"
#include "../output.hpp"
#include "../util.hpp"

enum class ConnectionType
{
    wireless,
    ethernet
};

ConnectionType parse_connection_type(std::string const& contype);

enum class ShowType
{
    none, // shows that the interface is active
    IPv4, // shows the ipv4 address if enabled
    IPv6, // shows the ipv6 address if enabled
    IPv6_fallback, // shows the ipv6 address if enabled, otherwise -> ipv4
    both // shows both addresses
};

ShowType parse_show_type(std::string const& showtype);

class Net : public StateItem
{
  private:
    // The interface name.
    std::string iface;
    // wireless/ethernet.
    ConnectionType iface_type;
    // What info to display.
    ShowType iface_show;

    // Whether it is enabled and active.
    bool iface_up;
    // ipv4 address.
    std::string iface_ipv4;
    // ipv6 address.
    std::string iface_ipv6;

    // Optional (wireless)
    std::string iface_essid;
    int iface_quality;
    int iface_bitrate;

    bool get_wireless_state(void);

    bool update(void) override;
    void print(std::ostream&, uint8_t) override;

    // addresses :: Map Interface (IPv4_Addr, IPv6_Addr)
    static std::map<std::string, std::pair<std::string, std::string>> addresses;
    static bool get_IP_addresses(Logger&);
    static time_t min_cooldown;

  public:
    explicit Net(JSON::Node const&);
    virtual ~Net(void) = default;
};

#endif
