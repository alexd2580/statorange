#ifndef __NETXYZHEADER_LOL___
#define __NETXYZHEADER_LOL___

#include <string>
#include <utility>
#include "../StateItem.hpp"
#include "../util.hpp"

enum ConnectionType
{
  Wireless,
  Ethernet
};

enum ShowType
{
  None,
  IPv4,
  IPv6,
  IPv6_Fallback,
  Both
};

class Net : public StateItem, public Logger
{
private:
  std::string iface;
  bool iface_up;
  ConnectionType iface_type;
  ShowType iface_show;
  std::string iface_ipv4;
  std::string iface_ipv6;

  // optional (wireless)
  std::string iface_essid;
  int iface_quality;
  int iface_bitrate;

  bool get_wireless_state(void);
  bool update(void);
  void print(void);
  // static:
  static std::map<std::string, std::pair<std::string, std::string>> addresses;
  static bool getIpAddresses(void);
  static time_t min_cooldown;

public:
  static void settings(JSONObject& section);
  explicit Net(JSONObject&);
  virtual ~Net();
};

#endif
