#ifndef __SYSTEM_STATE_HEADER___
#define __SYSTEM_STATE_HEADER___

#include<string>

#include<pthread.h>
#include<unistd.h>

typedef char const cchar;

typedef struct IPv4Address_ IPv4Address;
struct IPv4Address_
{
  uint8_t ip1;
  uint8_t ip2;
  uint8_t ip3;
  uint8_t ip4;
};

/**
 * Supports a max of two arguments. 
 * If the command has no arguments, set them to NULL
 * I was to lazy to implement va_stuff
 */
std::string execute(cchar* command, cchar* arg1, cchar* arg2);

enum BatStatus
{
  NotFound, Charging, Full, Discharging
};

class SystemState
{
private:
  pthread_mutex_t mutex;
  
public:
  SystemState();
  ~SystemState();
  
  //VOLUME
  bool mute;
  int volume;
  
  //DATE & TIME
  std::string time;
  
  //CPU
  int cpu_temp;
  float cpu_load;
  
  //BAT
  BatStatus bat_status;
  long bat_discharge;
  long bat_capacity;
  long bat_level;
  
  //NET
  bool net_eth_up;
  IPv4Address net_eth_ip;
  bool net_wlan_up;
  std::string net_wlan_essid;
  int net_wlan_quality;
  IPv4Address net_wlan_ip;
  
  //SPACE USAGE
  std::string space_home_size;
  std::string space_home_used;
  std::string space_root_size;
  std::string space_root_used;
  
  void update(void);
  void updateAll(void);
};

#endif
