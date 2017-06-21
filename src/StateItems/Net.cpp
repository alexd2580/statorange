#include <cstdlib>
#include <cstring>
#include <ostream>

#include "Net.hpp"

using namespace std;

time_t Net::min_cooldown = 1000; // TODO magicnumber
map<string, pair<string, string>> Net::addresses;

/******************************************************************************/
/******************************************************************************/

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/types.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
bool Net::get_IP_addresses(Logger& logger)
{
    static time_t last_updated = 0;
    time_t now = time(nullptr);
    if(now < last_updated + min_cooldown)
        return true;

    last_updated = now;

    addresses.clear();

    struct ifaddrs* base;
    struct ifaddrs* ifa;
    void* addr_ptr;

    if(getifaddrs(&base) != 0)
        return true;
    ifa = base;

    while(ifa != nullptr)
    {
        if(ifa->ifa_addr)
        {
            string iface(ifa->ifa_name);
            logger.log() << "Found interface " << iface << endl;
            auto family = ifa->ifa_addr->sa_family;
            if(family == AF_INET || family == AF_INET6)
            {
                if(addresses.find(iface) == addresses.end())
                    addresses[iface] = pair<string, string>("", "");
                auto& address_entry = addresses[iface];

                if(family == AF_INET)
                {
                    addr_ptr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                    char buffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, addr_ptr, buffer, INET_ADDRSTRLEN);
                    address_entry.first = string(buffer);
                    logger.log()
                        << "IPv4 address: " << address_entry.first << endl;
                }
                else if(family == AF_INET6)
                {
                    addr_ptr =
                        &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
                    char buffer[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, addr_ptr, buffer, INET6_ADDRSTRLEN);
                    address_entry.second = string(buffer);
                    logger.log()
                        << "IPv6 address: " << address_entry.second << endl;
                }
            }
        }
        ifa = ifa->ifa_next;
    }
    if(base != nullptr)
        freeifaddrs(base);
    return true;
}
#pragma clang diagnostic pop

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

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
        int const ESSID_LENGTH = 32;
        char buffer[ESSID_LENGTH];
        memset(buffer, 0, ESSID_LENGTH);
        wreq.u.essid.pointer = buffer;
        wreq.u.essid.length = ESSID_LENGTH; // with or without nulltermination?

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
            iface_quality = 100 * (dbm - DBM_MIN) / (DBM_MAX - DBM_MIN);
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

        iface_bitrate = wreq.u.bitrate.value / 1000000;

        /*out << "ESSID: " << essid << endl;
        out << "Quality: " << quality << "% (" << dbm << ")" << endl;
        out << "Bitrate: " << bitrate << "Mbit" << endl;*/
        throw true;
    }
    catch(bool b)
    {
        close(sockfd);
        return b;
    }
}

Net::Net(JSON const& item) : StateItem(item)
{
    iface.assign(item["interface"]);

    string contype = item["type"];
    if(contype.compare("wireless") == 0)
        iface_type = ConnectionType::wireless;
    else if(contype.compare("ethernet") == 0)
        iface_type = ConnectionType::ethernet;

    string showtype = item["show"];
    if(showtype.compare("ipv4") == 0)
        iface_show = ShowType::IPv4;
    else if(showtype.compare("ipv6") == 0)
        iface_show = ShowType::IPv6;
    else if(showtype.compare("both") == 0)
        iface_show = ShowType::both;
    else if(showtype.compare("none") == 0)
        iface_show = ShowType::none;
    else if(showtype.compare("ipv6_fallback") == 0)
        iface_show = ShowType::IPv6_fallback;

    time_t this_cooldown = item["cooldown"];
    min_cooldown = min(min_cooldown, this_cooldown);
}

bool Net::update(void)
{
    if(!get_IP_addresses(*this))
        return false;

    auto it = addresses.find(iface);
    if(it == addresses.end())
    {
        iface_up = false;
        return true;
    }
    iface_up = true;

    iface_ipv4.assign(it->second.first);
    iface_ipv6.assign(it->second.second);

    if(iface_type == ConnectionType::wireless)
        return get_wireless_state();

    return true;
}

void Net::print(ostream& out, uint8_t)
{
    if(iface_up)
    {
        BarWriter::separator(
            out, BarWriter::Separator::left, BarWriter::Coloring::neutral);
        switch(iface_type)
        {
        case ConnectionType::ethernet:
            out << icon << ' ' << iface;
            break;
        case ConnectionType::wireless:
            out << icon << ' ' << iface_essid << '(' << iface_quality << "%) ";
            BarWriter::separator(
                out, BarWriter::Separator::left, BarWriter::Coloring::neutral);
            break;
        }

        switch(iface_show)
        {
        case ShowType::IPv6_fallback:
            if(iface_ipv6.size() != 0)
                out << ' ' << iface_ipv6 << ' ';
            else if(iface_ipv4.size() != 0)
                out << ' ' << iface_ipv4 << ' ';
            else
                out << " No IPv4/IPv6 address ";
            break;
        case ShowType::both:
            if(iface_ipv4.size() != 0)
                out << ' ' << iface_ipv4 << ' ';
            BarWriter::separator(
                out, BarWriter::Separator::left, BarWriter::Coloring::neutral);
            out << ' ' << iface_ipv6 << ' ';
            break;
        case ShowType::IPv4:
            out << ' ' << iface_ipv4 << ' ';
            break;
        case ShowType::IPv6:
            out << ' ' << iface_ipv6 << ' ';
            break;
        case ShowType::none:
            out << " Up ";
            break;
        }
        BarWriter::separator(
            out, BarWriter::Separator::left, BarWriter::Coloring::white_on_black);
    }
}
