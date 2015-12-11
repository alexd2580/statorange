#ifndef __NETXYZHEADER_LOL___
#define __NETXYZHEADER_LOL___

#include<string>
#include<utility>
#include"../StateItem.hpp"
#include"../util.hpp"

struct IPv4Address
{
  uint8_t ip1;
  uint8_t ip2;
  uint8_t ip3;
  uint8_t ip4;
};

std::ostream& operator<<(std::ostream& out, IPv4Address ip);

enum ConnectionType
{
  Wireless, Ethernet
};

class Net : public StateItem, public Logger
{
private:
  //NET
  std::string iface;
  std::string ifstat_file_loc;
  ConnectionType type;
  bool iface_up;
  IPv4Address iface_ip;

  //optional
  std::string iface_essid;
  int iface_quality;

  static std::pair<std::string,std::string> ifstat_file_gen;
  static std::string ifconfig_file_loc;
  static std::string iwconfig_file_loc;

  bool getIpAddress(void);
  bool getWirelessState(void);

  bool update(void);
  void print(void);
public:
  static void settings(JSONObject& section);
  explicit Net(JSONObject&);
  virtual ~Net();
};

#endif
