
#define _POSIX_C_SOURCE 200809L

#include<cstdlib>
#include<cstring>
#include<iostream>
#include<csignal>
#include<ctime>
#include<cerrno>

#include<unistd.h>
#include<sys/wait.h>

#include"systemState.hpp"
#include"jsonParser.hpp"
#include"strUtil.hpp"

using namespace std;

string getDate = "date +%Y-%m-%d\\ %H:%M";
string getSpace = "df -h ";

string ifconfig_file_loc = "/sbin/ifconfig ";
string iwconfig_file_loc = "/sbin/iwconfig";
string shell_file_loc = "/bin/sh";
string temp_file_loc = "/sys/bus/acpi/devices/LNXTHERM:00/thermal_zone/temp";
string load_file_loc = "/proc/loadavg";
string bat_file_loc = "/sys/class/power_supply/BAT0/uevent";
string ifstat_file_loc = "/sys/class/net/%s/operstate";

string ethernet = "eth0";
string wireless = "wlan0";

string home_dir = "/home";
string root_dir = "/";


string execute(string command, cchar* arg1, cchar* arg2)
{
  int fd[2];
  pipe(fd);
  int childpid = fork();
  if(childpid == -1) //Fail
  {
     cerr << "FORK failed" << endl;
     return 0;
  } 
  else if(childpid == 0) //I am child
  {
    close(1); //stdout
    dup2(fd[1], 1);
    close(fd[0]);
    execlp(shell_file_loc.c_str(), shell_file_loc.c_str(), "-c", command.c_str(), arg1, arg2, (char*)nullptr);
    //child has been replaced by shell command
  }

  wait(nullptr);
  
  string res = "";
  char buffer[500];
  
  ssize_t n = 0;
  errno = EINTR;
  while(errno == EAGAIN || errno == EINTR || n>0)
  {
    errno = 0;
    n = read(fd[0], buffer, 500);
    if(n == 0)
      break;
    res += buffer;
  }
  return res;
}

/******************************************************************************/
/******************************************************************************/

std::ostream& operator<<(std::ostream& out, IPv4Address ip)
{
	return out << ip.ip1 << '.' << ip.ip2 << '.' << ip.ip3 << '.' << ip.ip4;
}

/**
 * Gets the ip address ot the given interface.
 */
IPv4Address getIpAddress(string interface)
{
  string command = ifconfig_file_loc + " " + interface;
  string output = execute(command, nullptr, nullptr);
  
  char const* c = output.c_str();
  while(*c != '\n') c++;
  skipWhitespaces(c);
  IPv4Address ip;
  int matched = sscanf(c, "inet addr:%hhu.%hhu.%hhu.%hhu", &ip.ip1, &ip.ip2, &ip.ip3, &ip.ip4);
  if(matched != 4)
    cerr << "Ip address not matched (" << matched << ")" << endl << c << endl;
  return ip;
}

/**
 * Queries iwconfig for the essid and the quality of the given interface
 * Returns the SSID.
 */
string getWirelessState(string interface, int& quality)
{
  string output = execute(iwconfig_file_loc + " " + interface, nullptr, nullptr);
  
  quality = -1;
  char const* c = output.c_str();
  string ssid;
  
  while(true)
  {
    skipWhitespaces(c);
    if(*c == '\0')
      break;
    else if(strncmp(c, "ESSID:", 6) == 0)
    {
      c+=7;
      char const* e = c;
      while(*e != '"')
        e++;
      
      ssid.assign(c, e-c);
    }
    else if(strncmp(c, "Link Quality=", 13) == 0)
    {
      c+=13;
      char * d;
      int q1 = (int)strtol(c, &d, 0);
      c = d+1;
      int q2 = (int)strtol(c, nullptr, 0);
      quality = 100*q1 / q2;
      break; //assuming that quality comes after essid
    }
    skipNonWhitespace(c);
  }
  
  return ssid;
}

/******************************************************************************/
/******************************************************************************/

void getSpaceUsage(string mount_point, string& size, string& used)
{
  string output = execute(getSpace + " " + mount_point, nullptr, nullptr);
  
  char const* c = output.c_str();
  while(*c != '\n') c++;
  c++;
  skipNonWhitespace(c);
  skipWhitespaces(c);
  char const* beg = c;
  skipNonWhitespace(c);
  size.assign(beg, c-beg);
  skipWhitespaces(c);
  beg = c;
  skipNonWhitespace(c);
  used.assign(beg, c-beg);
  return;
}

/******************************************************************************/
/******************************************************************************/

SystemState::SystemState()
{
  pthread_mutex_init(&mutex, nullptr);
  pthread_mutex_lock(&mutex);
  
  mute = false;
  volume = 0;
  
  cpu_temp = 0;
  cpu_load = 0.0f;
  bat_status = NotFound;
  bat_discharge = 0;
  bat_capacity = 0;
  bat_level = 0;
  
  net_eth_up = false;
  net_wlan_up = false;
  net_wlan_quality = 0;
  
  pthread_mutex_unlock(&mutex);
}


SystemState::~SystemState()
{
  pthread_mutex_destroy(&mutex);
}

/**
 * Queries the system resources
 */
void SystemState::update(void)
{
  pthread_mutex_lock(&mutex);

  char line[1000] = { 0 };
  string tmp;
  
  //Date
  time = execute(getDate, nullptr, nullptr);
  
  //Temp
  FILE* tfile = fopen(temp_file_loc.c_str(), "r");
  fgets(line, 10, tfile);
  fclose(tfile);
  cpu_temp = (int)strtol(line, nullptr, 0) / 1000;
  
  FILE* lfile = fopen(load_file_loc.c_str(), "r");
  fgets(line, 10, lfile);
  fclose(lfile);
  cpu_load = strtof(line, nullptr);
  
  //battery
  FILE* bfile = fopen(bat_file_loc.c_str(), "r");
  if(bfile == nullptr)
    bat_status = NotFound; //Probably no battery
  else
  {
    while(fgets(line, 200, bfile) != nullptr)
    {
      if(strncmp(line+13, "STATUS", 6) == 0)
      {
        if(strncmp(line+20, "Full", 4) == 0)
          bat_status = Full;
        else if(strncmp(line+20, "Charging", 8) == 0)
          bat_status = Charging;
        else if(strncmp(line+20, "Discharging", 11) == 0)
          bat_status = Discharging;
        else
          bat_status = Full;
      }
      else if(strncmp(line+13, "POWER_NOW", 9) == 0)
        bat_discharge = strtol(line+23, NULL, 0);
      else if(strncmp(line+13, "ENERGY_FULL_DESIGN", 18) == 0)
        bat_capacity = strtol(line+32, NULL, 0);
      else if(strncmp(line+13, "ENERGY_NOW", 10) == 0)
        bat_level = strtol(line+24, NULL, 0);
    }
    fclose(bfile);
  }
  
  tmp = ifstat_file_loc + ethernet;
  FILE* ethfile = fopen(tmp.c_str(), "r");
  fgets(line, 10, ethfile);
  fclose(ethfile);
  if((net_eth_up = strncmp(line, "up", 2) == 0))
    net_eth_ip = getIpAddress(ethernet);

  tmp = ifstat_file_loc + wireless;
  FILE* wlanfile = fopen(tmp.c_str(), "r");
  fgets(line, 10, wlanfile);
  fclose(wlanfile);
  if((net_wlan_up = strncmp(line, "up", 2) == 0))
  {
    net_wlan_ip = getIpAddress(wireless);
    net_wlan_essid = getWirelessState(wireless, net_wlan_quality);
  }
  
  getSpaceUsage(root_dir, space_root_size, space_root_used);
  getSpaceUsage(home_dir, space_home_size, space_home_used);

  pthread_mutex_unlock(&mutex);
}

void update_constSystemState(SystemState* sys)
{
  (void)sys;
  getSpaceUsage(root_dir, sys->space_root_size, sys->space_root_used);
  getSpaceUsage(home_dir, sys->space_home_size, sys->space_home_used);
}


