
#include<iostream>
#include<cstdlib>
#include<cstring>

#include"Net.hpp"
#include"../output.hpp"

using namespace std;

ostream& operator<<(ostream& out, IPv4Address ip)
{
	return out << (int)ip.ip1 << '.' << (int)ip.ip2 << '.' << (int)ip.ip3 << '.' << (int)ip.ip4;
}

pair<string,string> Net::ifstat_file_gen = pair<string, string>("","");
string Net::ifconfig_file_loc = "";
string Net::iwconfig_file_loc = "";

/******************************************************************************/
/******************************************************************************/

void Net::settings(JSONObject& section)
{
	ifstat_file_gen.first = section["ifstat_file_pre"].string();
	ifstat_file_gen.second = section["ifstat_file_post"].string();
	ifconfig_file_loc = section["ifconfig_file"].string();
	iwconfig_file_loc = section["iwconfig_file"].string();
}

bool Net::getIpAddress(void)
{
  string cmd = ifconfig_file_loc + ' ' + iface;
  string output = execute(cmd);

  char const* c = output.c_str();
  while(*c != '\n') c++;
  skipWhitespaces(c);
  int matched = sscanf(c, "inet %hhu.%hhu.%hhu.%hhu", &iface_ip.ip1, &iface_ip.ip2, &iface_ip.ip3, &iface_ip.ip4); // inet addr:
  if(matched != 4)
  {
    log() << "Ip address not matched (" << matched << ")" << endl << c << endl;
    return false;
  }
  return true;
}

bool Net::getWirelessState(void)
{
  string command = iwconfig_file_loc + ' ' + iface;
  string output = execute(command);

  iface_quality = -1;
  char const* c = output.c_str();

  while(true)
  {
    skipWhitespaces(c);
    if(*c == '\0')
      return false;
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
      return true;
    }
    skipNonWhitespace(c);
  }
  return false;
}

Net::Net(JSONObject& item) :
  StateItem(item), Logger("[Net]", cerr)
{
	iface.assign(item["interface"].string());
	ifstat_file_loc.assign(ifstat_file_gen.first + iface + ifstat_file_gen.second);
	string contype = item["type"].string();
	if(contype.compare("wireless") == 0)
		type = Wireless;
	else if(contype.compare("ethernet") == 0)
		type = Ethernet;
}

Net::~Net()
{
}

bool Net::update(void)
{
  char line[11] = { 0 };
  FILE* upfile = fopen(ifstat_file_loc.c_str(), "r");
  FAIL_ON_FALSE(upfile != nullptr)
  fgets(line, 10, upfile);
  fclose(upfile);

  if((iface_up = (strncmp(line, "up", 2) == 0)))
  {
    FAIL_ON_FALSE(getIpAddress())
    if(type == Wireless)
      FAIL_ON_FALSE(getWirelessState())
  }

  return true;
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
      print_icon(icon_wlan);
      cout << ' ' << iface_essid << '(' << iface_quality << "%%) ";
      separate(Left, neutral_colors);
      cout << ' ' << iface_ip << ' ';
      break;
    default:
			cout << " Something went wrong ";
      break;
    }
    separate(Left, white_on_black);
  }
}
