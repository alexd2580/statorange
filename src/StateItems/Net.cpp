
#include<iostream>
#include<cstdlib>
#include<cstring>

#include"Net.hpp"
#include"../output.hpp"

using namespace std;

time_t Net::min_cooldown = 1000; // TODO magicnumber
map<string, pair<string, string>> Net::addresses;

/******************************************************************************/
/******************************************************************************/

#include<sys/types.h>
#include<ifaddrs.h>
#include<netinet/in.h>
#include<arpa/inet.h>

bool Net::getIpAddresses(void)
{
		static time_t last_updated = 0;
		time_t now = time(nullptr);
		if(now < last_updated + min_cooldown)
			return true;

		last_updated = now;

		addresses.clear();

    struct ifaddrs* base;
    struct ifaddrs* ifa;
    void* tmpAddrPtr;

		string iface;
		string ipv4;
		string ipv6;

    if(getifaddrs(&base) != 0)
      return true;
		ifa = base;

    while(ifa != nullptr)
    {
        if(ifa->ifa_addr)
				{
					iface = string(ifa->ifa_name);

	        if (ifa->ifa_addr->sa_family == AF_INET)
	        {
	            tmpAddrPtr= &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
	            char addressBuffer[INET_ADDRSTRLEN];
	            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
							ipv4 = string(addressBuffer);
	        }
	        else if (ifa->ifa_addr->sa_family == AF_INET6)
	        {
	            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
	            char addressBuffer[INET6_ADDRSTRLEN];
	            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
							ipv6 = string(addressBuffer);
	        }

					if(ipv4.size() > 0 || ipv6.size() > 0)
						addresses[iface] = pair<string, string>(ipv4, ipv6);
				}
        ifa = ifa->ifa_next;
    }
    if(base != nullptr)
			freeifaddrs(base);
    return true;
}

#include<cstdio>
#include<cerrno>
#include<sys/ioctl.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<linux/wireless.h>

bool Net::get_wireless_state(void)
{
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd == -1)
  {
      log() << "Cannot open socket" << endl;
			log_errno();
      return false;
  }

	try
	{
    struct iwreq wreq;
    memset(&wreq, 0, sizeof(struct iwreq));
    snprintf(wreq.ifr_name, 7, "wlp2s0");

    /*** ESSID ***/
    char buffer[32];
    memset(buffer, 0, 32);
    wreq.u.essid.pointer = buffer;
    wreq.u.essid.length = 32;

    if(ioctl(sockfd, SIOCGIWESSID, &wreq) == -1)
    {
        log() << "SIOCGIWESSID ioctl failed" << endl;
        log_errno();
        throw false;
    }

    iface_essid.assign((char*)wreq.u.essid.pointer);

    /*** QUALITY ***/
    iw_statistics stats;
    wreq.u.data.pointer = &stats;
    wreq.u.data.length = sizeof(iw_statistics);

    if(ioctl(sockfd, SIOCGIWSTATS, &wreq) == -1)
    {
      log() << "SIOCGIWSTATS ioctl failed" << endl;
      log_errno();
      throw false;
    }

    if(stats.qual.updated & IW_QUAL_DBM)
    {
        #define DBM_MIN (-100)
        #define DBM_MAX (-20)
        int dbm = stats.qual.level - 256;
        iface_quality = 100*(dbm-DBM_MIN) / (DBM_MAX-DBM_MIN);
    }
    else
    {
        log() << "Cannot read quality" << endl;
				iface_quality = -100;
    }

    /*** BITRATE ***/
    if(ioctl(sockfd, SIOCGIWRATE, &wreq) == -1)
    {
      log() << "SIOCGIWRATE ioctl failed" << endl;
			log_errno();
      throw false;
    }

    iface_bitrate = wreq.u.bitrate.value/1000000;

    /*cout << "ESSID: " << essid << endl;
    cout << "Quality: " << quality << "% (" << dbm << ")" << endl;
    cout << "Bitrate: " << bitrate << "Mbit" << endl;*/
		throw true;
	}
	catch(bool b)
	{
    shutdown(sockfd, SHUT_RDWR);
		return b;
	}
}

/*
bool Net::getWirelessState(void)
{
  string command = iwconfig_file_loc + ' ' + iface;
  string output = execute(command);

  iface_quality = -1;
  TextPos pos(output.c_str());

  while(true)
  {
    pos.skip_whitespace();
    if(*pos == '\0')
      return false;
    else if(strncmp(pos.ptr(), "ESSID:", 6) == 0)
    {
      pos.offset(7);
			char const * c = pos.ptr();
      char const* e = c;
      while(*e != '"')
        e++;

      iface_essid.assign(c, e-c);
    }
    else if(strncmp(pos.ptr(), "Link Quality=", 13) == 0)
    {
      pos.offset(13);
      double q1 = pos.parse_num();
      (void)pos.next();
      double q2 = pos.parse_num();
      iface_quality = (int)(100*q1 / q2);
      return true;
    }
    pos.skip_nonspace();
  }
  return false;
}*/

Net::Net(JSONObject& item) :
  StateItem(item), Logger("[Net]", cerr)
{
	iface.assign(item["interface"].string());

	string contype = item["type"].string();
	if(contype.compare("wireless") == 0)
		iface_type = Wireless;
	else if(contype.compare("ethernet") == 0)
		iface_type = Ethernet;

	string showtype = item["show"].string();
	if(showtype.compare("ipv4") == 0)
		iface_show = IPv4;
	else if(showtype.compare("ipv6") == 0)
		iface_show = IPv6;
	else if(showtype.compare("both") == 0)
		iface_show = Both;
	else if(showtype.compare("none") == 0)
		iface_show = None;
	else if(showtype.compare("ipv6_fallback") == 0)
		iface_show = IPv6_Fallback;

	time_t this_cooldown = item["cooldown"].number();
	min_cooldown = min(min_cooldown, this_cooldown);
}

Net::~Net()
{
}

bool Net::update(void)
{
	FAIL_ON_FALSE(getIpAddresses())

	auto it = addresses.find(iface);
	if(it == addresses.end())
	{
		iface_up = false;
		return true;
	}
	iface_up = true;

	iface_ipv4.assign(it->second.first);
	iface_ipv6.assign(it->second.second);

  if(iface_type == Wireless)
    FAIL_ON_FALSE(get_wireless_state())

  return true;
}

void Net::print(void)
{
  if(iface_up)
  {
    separate(Left, neutral_colors);
    switch(iface_type)
    {
    case Ethernet:
      cout << ' ' << iface;
      break;
    case Wireless:
      print_icon(icon_wlan);
      cout << ' ' << iface_essid << '(' << iface_quality << "%%) ";
      separate(Left, neutral_colors);
      break;
    default:
			cout << " Something went wrong ";
			return;
    }

		switch(iface_show)
		{
		case IPv6_Fallback:
			if(iface_ipv6.size() == 0)
				cout << ' ' << iface_ipv4 << ' ';
			else
				cout << ' ' << iface_ipv6 << ' ';
			break;
		case Both:
			cout << ' ' << iface_ipv4 << ' ';
			separate(Left, neutral_colors);
			cout << ' ' << iface_ipv6 << ' ';
			break;
		case IPv4:
			cout << ' ' << iface_ipv4 << ' ';
			break;
		case IPv6:
			cout << ' ' << iface_ipv6 << ' ';
			break;
		case None:
		default:
			cout << " Up ";
			break;
		}
    separate(Left, white_on_black);
  }
}
