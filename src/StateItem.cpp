#include <climits>
#include <iostream>
#include <memory>
#include <sstream>

#include "StateItem.hpp"
#include "StateItems/Battery.hpp"
#include "StateItems/CPU.hpp"
#include "StateItems/Date.hpp"
#include "StateItems/I3Workspaces.hpp"
#include "StateItems/IMAPMail.hpp"
#include "StateItems/Net.hpp"
#include "StateItems/Space.hpp"
#include "StateItems/Volume.hpp"
#include "output.hpp"

using namespace std;
using JSON::Number;

chrono::seconds StateItem::min_cooldown(std::numeric_limits<int>::max());
vector<unique_ptr<StateItem>> StateItem::left_items;
vector<unique_ptr<StateItem>> StateItem::center_items;
vector<unique_ptr<StateItem>> StateItem::right_items;
bool StateItem::show_failed_modules;
map<int, StateItem*> StateItem::event_sockets;

BarWriter::Icon StateItem::parse_icon_from_json(JSON::Node const& item)
{
    return BarWriter::parse_icon(item["icon"].string());
}

StateItem::StateItem(JSON::Node const& item)
    : Logger(item["item"].string()),
      module_name(item["item"].string()),
      cooldown(chrono::seconds(item["cooldown"].number<uint32_t>())),
      icon(parse_icon_from_json(item)),
      button(item["button"].exists() && item["button"].string().length() > 0),
      button_command(item["button"].string())
{
    min_cooldown = min(min_cooldown, cooldown);
    last_updated = chrono::system_clock::time_point::min();
    valid = false;
    cached = false;
}

void StateItem::register_event_socket(int fd)
{
    if(event_sockets.find(fd) != event_sockets.end())
    {
        log() << "Can't register event socket, socket already registered."
              << endl;
        return;
    }
    event_sockets[fd] = this;
}

bool StateItem::handle_events(int)
{
    log() << "No `handle_events` defined for this `StateItem`!" << endl;
    return false;
}

void StateItem::unregister_event_socket(int fd) const
{
    event_sockets.erase(fd);
}

void StateItem::wrap_update(void)
{
    auto now = chrono::system_clock::now();
    if(now > last_updated + cooldown)
    {
        force_update();
    }
}

void StateItem::force_update(void)
{
    last_updated = chrono::system_clock::now();
    valid = update();
    cached = false;
}

void StateItem::wrap_print(ostream& out, uint8_t display_number)
{
    if(!cached)
    {
        ostringstream cache;
        if(valid)
        {
            if(button)
                BarWriter::button(cache, button_command, [=](ostream& ostr) {
                    print(ostr, display_number);
                });
            else
                print(cache, display_number);
        }
        else if(show_failed_modules)
        {
            BarWriter::separator(
                cache,
                BarWriter::Separator::vertical,
                BarWriter::Coloring::warn);
            cache << " Module " << module_name << " failed ";
            BarWriter::separator(
                cache,
                BarWriter::Separator::vertical,
                BarWriter::Coloring::white_on_black);
        }

        print_string = cache.str();
        cached = true;
    }
    out << print_string;
}

StateItem* StateItem::init_item(JSON::Node const& json_item)
{
    try
    {
        string const& item = json_item["item"].string();
        if(item == "CPU")
            return new CPU(json_item);
        else if(item == "Battery")
            return new Battery(json_item);
        else if(item == "Net")
            return new Net(json_item);
        else if(item == "Date")
            return new Date(json_item);
        else if(item == "Volume")
            return new Volume(json_item);
        else if(item == "Space")
            return new Space(json_item);
        else if(item == "IMAPMail")
            return new IMAPMail(json_item);
        else if(item == "I3Workspaces")
            return new I3Workspaces(json_item);
    }
    catch(string const& error)
    {
        Logger::log("StateItem") << error << endl;
    }

    // If no name matches - ignore.
    return nullptr;
}

void StateItem::init_section(
    JSON::Node const& config,
    string const& section_name,
    vector<unique_ptr<StateItem>>& section)
{
    if(config[section_name].exists())
    {
        for(auto const& json_item : config[section_name].array())
        {
            auto new_item = init_item(json_item);
            if(new_item != nullptr)
                section.emplace_back(new_item);
        }
    }
}

void StateItem::init(JSON::Node const& config)
{
    init_section(config, "left", left_items);
    init_section(config, "center", center_items);
    init_section(config, "right", right_items);

    show_failed_modules = config["show failed modules"].boolean();
}

void StateItem::update_all(void)
{
    for(auto& state : left_items)
        state->wrap_update();
    for(auto& state : center_items)
        state->wrap_update();
    for(auto& state : right_items)
        state->wrap_update();
}

void StateItem::force_update_all(void)
{
    for(auto& state : left_items)
        state->force_update();
    for(auto& state : center_items)
        state->force_update();
    for(auto& state : right_items)
        state->force_update();
}

void StateItem::print_section(
    ostream& out,
    BarWriter::Alignment a,
    vector<unique_ptr<StateItem>> const& section,
    uint8_t display_number)
{
    auto printer = [&section, display_number](ostream& ostr) {
        for(auto& state : section)
            state->wrap_print(ostr, display_number);
    };
    BarWriter::align(out, a, printer);
}

void StateItem::print_state(ostream& out, uint8_t display)
{
    print_section(out, BarWriter::Alignment::left, left_items, display);
    print_section(out, BarWriter::Alignment::center, center_items, display);
    print_section(out, BarWriter::Alignment::right, right_items, display);
}

void StateItem::wait_for_events(void)
{
    fd_set read_fds;
    struct timeval tv;

    int max_fd = 0;
    FD_ZERO(&read_fds);
    for(auto const& event_socket : event_sockets)
    {
        int fd = event_socket.first;
        FD_SET(fd, &read_fds);
        max_fd = max(max_fd, fd);
    }

    tv.tv_sec = StateItem::min_cooldown.count();
    tv.tv_usec = 0;
    Logger::log("StateItem")
        << "Setting socket timeout to " << (int)tv.tv_sec << " seconds" << endl;

    int select_res = select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
    switch(select_res)
    {
    case -1:
        perror("select");
        break;
    case 0:
        // Timeout exceeded
        Logger::log("StateItem") << "Timeout" << endl;
        break;
    default:
        Logger::log("StateItem")
            << (int)select_res << " sockets are ready for reading" << endl;
        for(auto const& event_socket : event_sockets)
        {
            if(FD_ISSET(event_socket.first, &read_fds))
            {
                auto item = event_socket.second;
                Logger::log("StateItem") << "Module " << item->module_name
                                         << " can process events on socket "
                                         << event_socket.first << endl;
                item->valid = item->handle_events(event_socket.first);
            }
        }
        break;
    }
}
