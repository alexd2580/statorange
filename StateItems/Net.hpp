#ifndef __NETXYZHEADER_LOL___
#define __NETXYZHEADER_LOL___

#include<string>
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
  std::string const iface;
  ConnectionType const type;
  bool iface_up;
  IPv4Address iface_ip;
  
  //optional
  std::string iface_essid;
  int iface_quality;
  
  static cchar* const ifstat_file_loc;
  static std::string ifconfig_file_loc;
  static std::string iwconfig_file_loc;
  
  void getIpAddress(void);
  void getWirelessState(void);

  void performUpdate(void);
  void print(void);
public:
  Net(std::string interface, ConnectionType);
  virtual ~Net();
};

#endif






  
  

