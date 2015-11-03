
#include<iostream>
#include<cstdlib>
#include<cstring>

#include"Net.hpp"
#include"../output.hpp"

using namespace std;

ostream& operator<<(ostream& out, IPv4Address ip)
{
	return out << ip.ip1 << '.' << ip.ip2 << '.' << ip.ip3 << '.' << ip.ip4;
}

cchar* const Net::ifstat_file_loc = "/sys/class/net/%s/operstate";
string Net::ifconfig_file_loc = "/sbin/ifconfig ";
string Net::iwconfig_file_loc = "/sbin/iwconfig ";

/******************************************************************************/
/******************************************************************************/

void Net::getIpAddress(void)
{
  string cmd = ifconfig_file_loc + ' ' + iface;
  string output = execute(cmd);
  
  char const* c = output.c_str();
  while(*c != '\n') c++;
  skipWhitespaces(c);
  int matched = sscanf(c, "inet addr:%hhu.%hhu.%hhu.%hhu", &iface_ip.ip1, &iface_ip.ip2, &iface_ip.ip3, &iface_ip.ip4);
  if(matched != 4)
    log() << "Ip address not matched (" << matched << ")" << endl << c << endl;
}

void Net::getWirelessState(void)
{
  string command = iwconfig_file_loc + iface;
  string output = execute(command);
  
  iface_quality = -1;
  char const* c = output.c_str();

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
      
      iface_essid.assign(c, e-c);
    }
    else if(strncmp(c, "Link Quality=", 13) == 0)
    {
      c+=13;
      char * d;
      int q1 = (int)strtol(c, &d, 0);
      c = d+1;
      int q2 = (int)strtol(c, nullptr, 0);
      iface_quality = 100*q1 / q2;
      break; //assuming that quality comes after essid
    }
    skipNonWhitespace(c);
  }
}

Net::Net(string interface, ConnectionType conType) :
  StateItem(10), Logger("[Net]", cerr), iface(interface), type(conType)
{
}

Net::~Net()
{
}

void Net::performUpdate(void)
{
  char file[50] = { 0 };
  sprintf(file, ifstat_file_loc, iface.c_str());
  char line[11] = { 0 };
  FILE* upfile = fopen(file, "r");
  fgets(line, 10, upfile);
  fclose(upfile);
  
  if((iface_up = (strncmp(line, "up", 2) == 0)))
  {
    getIpAddress();
    if(type == Wireless)
      getWirelessState();
  }
}

void Net::print(void)
{
  if(iface_up)
  {
    separate(Left, neutral_colors);
    switch(type)
    {
    case Ethernet:
      cout << ' ' << iface << ' ' << iface_ip << ' ';
      break;
    case Wireless:
      PRINT_ICON(icon_wlan);
      cout << ' ' << iface_essid << '(' << iface_quality << "%%) ";
      separate(Left, neutral_colors);
      cout << ' ' << iface_ip << ' ';
      break;
    default:
      break;
    }
    separate(Left, white_on_black);
  }
}
