#include "output.hpp"
#include <iostream>

#include "StateItem.hpp"
#include "StateItems/Battery.hpp"
#include "StateItems/CPU.hpp"
#include "StateItems/Date.hpp"
#include "StateItems/IMAPMail.hpp"
#include "StateItems/Net.hpp"
#include "StateItems/Space.hpp"
#include "StateItems/Volume.hpp"

using namespace std;

chrono::seconds StateItem::min_cooldown;
vector<StateItem*> StateItem::states;

StateItem::StateItem(string const& logname, JSON const& item)
    : Logger(logname), last_updated(chrono::system_clock::time_point::min())
{
    module_name.assign(item["item"]);
    cooldown = chrono::seconds(item["cooldown"]);
    min_cooldown = min(min_cooldown, cooldown);
    button = false;

    if(item.has("button"))
    {
        button_command.assign(item["button"]);
        button = true;
    }
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
        last_updated = now;
        valid = update();
    }
}

void StateItem::force_update(void)
{
    last_updated = chrono::system_clock::now();
    valid = update();
}

void StateItem::wrap_print(void)
{
    if(valid)
    {
        if(button)
        {
            startButton(button_command, cout);
            print();
            stopButton(cout);
        }
        else
            print();
    }
    else
    {
        separate(Direction::left, Color::warn);
        cout << " Module " << module_name << " failed ";
        separate(Direction::left, Color::white_on_black);
    }
}

void StateItem::init(JSON const& config)
{
    auto& order = config["order"];
    auto length = order.size();

    for(decltype(length) i = 0; i < length; i++)
    {
        auto& section = order[i];
        string item = section["item"];
        if(item.compare("CPU") == 0)
            states.push_back(new CPU(section));
        else if(item.compare("Battery") == 0)
            states.push_back(new Battery(section));
        else if(item.compare("Net") == 0)
            states.push_back(new Net(section));
        else if(item.compare("Date") == 0)
            states.push_back(new Date(section));
        else if(item.compare("Volume") == 0)
            states.push_back(new Volume(section));
        else if(item.compare("Space") == 0)
            states.push_back(new Space(section));
        else if(item.compare("IMAPMail") == 0)
            states.push_back(new IMAPMail(section));
    }
}

void StateItem::update_all(void)
{
    for(auto state : states)
        state->wrap_update();
}

void StateItem::force_update_all(void)
{
    for(auto state : states)
        state->force_update();
}

void StateItem::print_state(void)
{
    for(auto state : states)
        state->wrap_print();
}

void StateItem::deinit(void)
{
    for(auto state : states)
        delete state;
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
