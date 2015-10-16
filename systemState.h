#ifndef __SYSTEM_STATE_HEADER___
#define __SYSTEM_STATE_HEADER___

#include<pthread.h>
#include<unistd.h>

typedef char const cchar;

/**
 * Supports a max of two arguments. 
 * If the command has no arguments, set them to NULL
 * I was to lazy to implement va_stuff
 */
ssize_t execute(cchar* command, cchar* arg1, cchar* arg2, char* buffer, size_t bufSize);

#define NO_BATTERY 0
#define BATTERY_CHARGING 1
#define BATTERY_FULL 2
#define BATTERY_DISCHARGING 3

typedef struct SystemState_ SystemState;
struct SystemState_
{
  pthread_mutex_t mutex;
  
  //VOLUME
  char mute;
  int volume;
  
  //DATE & TIME
  char time[100];
  
  //CPU
  int cpu_temp;
  float cpu_load;
  
  //BAT
  char bat_status;
  long bat_discharge;
  long bat_capacity;
  long bat_level;
  
  //NET
  char net_eth_up;
  char net_eth_ip[40];
  char net_wlan_up;
  char net_wlan_essid[100];
  int net_wlan_quality;
  char net_wlan_ip[40];
  
  //SPACE USAGE
  char space_home_size[10];
  char space_home_used[10];
  char space_root_size[10];
  char space_root_used[10];
};

void init_systemState(SystemState* sys);
void update_systemState(SystemState* sys);
void update_constSystemState(SystemState* sys);
void free_systemState(SystemState* sys);

#endif
