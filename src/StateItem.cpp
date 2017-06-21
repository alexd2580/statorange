#include <climits>
#include <iostream>
#include <sstream>
#include <memory>

#include "StateItem.hpp"
#include "StateItems/Battery.hpp"
#include "StateItems/CPU.hpp"
#include "StateItems/Date.hpp"
#include "StateItems/IMAPMail.hpp"
#include "StateItems/Net.hpp"
#include "StateItems/Space.hpp"
#include "StateItems/Volume.hpp"
#include "output.hpp"

using namespace std;

chrono::seconds StateItem::min_cooldown(std::numeric_limits<int>::max());
vector<unique_ptr<StateItem>> StateItem::left_items;
vector<unique_ptr<StateItem>> StateItem::center_items;
vector<unique_ptr<StateItem>> StateItem::right_items;
map<int, StateItem*> StateItem::event_sockets;

StateItem::StateItem(JSON const& item)
    : Logger(item["item"]),
      module_name(item["item"]),
      cooldown(chrono::seconds((long)item["cooldown"])),
      icon(BarWriter::parse_icon(item.get("icon").as_string_with_default(""))),
      button(item.has("button")),
      button_command(item.get("button").as_string_with_default(""))
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

void StateItem::handle_events(void)
{
    log() << "No `handle_events` defined for this `StateItem`!" << endl;
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
        else
        {
            BarWriter::separator(
                cache, BarWriter::Separator::left, BarWriter::Coloring::warn);
            cache << " Module " << module_name << " failed ";
            BarWriter::separator(
                cache,
                BarWriter::Separator::left,
                BarWriter::Coloring::white_on_black);
        }

        print_string = cache.str();
        cached = true;
    }
    out << print_string;
}

StateItem* StateItem::init_item(JSON const& json_item)
{
    string item = json_item["item"];
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

    // If no name matches - ignore.
    return nullptr;
}

void StateItem::init_section(
    JSON const& config,
    string const& section_name,
    vector<unique_ptr<StateItem>>& section)
{
    if(config.has(section_name))
    {
        for(auto const& json_item_uptr : config[section_name].as_vector())
        {
            auto new_item = init_item(*json_item_uptr);
            if(new_item != nullptr)
                section.emplace_back(new_item);
        }
    }
}

void StateItem::init(JSON const& config)
{
    init_section(config, "left", left_items);
    init_section(config, "center", center_items);
    init_section(config, "right", right_items);
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

    int select_res = select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
    switch(select_res)
    {
    case -1:
        perror("select");
        break;
    case 0:
        // Timeout exceeded
        break;
    default:
        for(auto const& event_socket : event_sockets)
        {
            if(FD_ISSET(event_socket.first, &read_fds))
            {
                event_socket.second->handle_events();
            }
        }
        break;
    }
}
