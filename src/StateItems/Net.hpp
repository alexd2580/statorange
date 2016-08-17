#ifndef __NETXYZHEADER_LOL___
#define __NETXYZHEADER_LOL___

#include <string>
#include <utility>

#include "../StateItem.hpp"
#include "../util.hpp"
#include "../output.hpp"

enum class ConnectionType
{
  wireless,
  ethernet
};

enum class ShowType
{
  none,          // shows that the interface is active
  IPv4,          // shows the ipv4 address if enabled
  IPv6,          // shows the ipv6 address if enabled
  IPv6_fallback, // shows the ipv6 address if enabled, otherwise -> ipv4
  both           // shows both addresses
};

class Net : public StateItem, public Logger
{
private:
  /** The interface name **/
  std::string iface;
  /** Whether it is enabled and active **/
  bool iface_up;
  /** wireless/ethernet **/
  ConnectionType iface_type;
  /** What info to display **/
  ShowType iface_show;
  /** ipv4 address **/
  std::string iface_ipv4;
  /** ipv6 address **/
  std::string iface_ipv6;

  // optional (wireless)
  Icon icon;
  std::string iface_essid;
  int iface_quality;
  int iface_bitrate;

  bool get_wireless_state(void);
  bool update(void);
  void print(void);

  // static
  /** addresses :: Map Interface (IPv4_Addr, IPv6_Addr) **/
  static std::map<std::string, std::pair<std::string, std::string>> addresses;
  static bool get_IP_addresses(void);
  static time_t min_cooldown;

public:
  explicit Net(JSON const&);
  virtual ~Net(void) = default;
};

#endif
