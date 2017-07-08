
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <sys/socket.h>

#include "../Application.hpp"
#include "../i3/i3-ipc-constants.hpp"
#include "../i3/i3-ipc.hpp"
#include "../json_parser.hpp"
#include "../output.hpp"
#include "../util.hpp"
// #include "I3State.hpp"
#include "I3Workspaces.hpp"

using namespace std;
using BarWriter::Separator;
using BarWriter::Coloring;

string I3Workspaces::get_window_name(JSON::Node const& container)
{
    string name(container["name"].string("ERROR"));
    ANON_LOG << "Requesting window name: " << name << endl;

    if(name.length() > 20)
    {
        string class_(container["window_properties"]["class"].string("ERROR"));
        ANON_LOG << "Too long window name, using class: " << class_ << endl;
        if(class_.length() > 20)
        {
            string sub = name.substr(0, 17);
            return sub + "[…]";
        }
        return class_;
    }
    return name;
}

bool I3Workspaces::invalid_event(void)
{
    log() << "Invalid packet received. Aborting" << endl;
    return false;
}

bool I3Workspaces::mode_event(char const* response)
{
    JSON::Node json(response);
    i3.mode.assign(json["change"].string());
    return true;
}

bool I3Workspaces::window_event(char const* response)
{
    JSON::Node json(response);
    string change = json["change"].string();

    auto& container = json["container"];
    uint64_t window_id = container["id"].number<uint64_t>();

    if(change == "new")
        ; // i3.new_window(window_id, get_window_name(container));
    else if(change == "title")
        ; // i3.rename_window(window_id, get_window_name(container));
    else if(change == "focus")
        ; // i3.focus_window(window_id);
    else if(change == "close")
        ; // i3.close_window(window_id);
    else if(change == "fullscreen_mode")
        log() << "Ignoring fullscreen mode event" << endl;
    else if(change == "move")
        log() << "Ignoring move event" << endl;
    else
        log() << "Unhandled window event type: " << change << endl;

    return true;
}

bool I3Workspaces::workspace_event(char const* response)
{
    JSON::Node json(response);
    string change(json["change"].string());

    auto& current = json["current"];
    uint8_t num = current["num"].number<uint8_t>();

    if(change == "init")
        i3.init_workspace(num, current["name"].string());
    else if(change == "focus")
        i3.focus_workspace(num);
    else if(change == "urgent")
        i3.urgent_workspace(num, current["urgent"].boolean());
    else if(change == "empty")
        i3.empty_workspace(num);
    else
        log() << "Unhandled workspace event type: " << change << endl;

    return true;
}

bool I3Workspaces::output_event(char const*)
{
    log() << "Output event - Restarting application" << endl;
    Application::dead = true;
    Application::exit_status = 1;
    return false;
}

bool I3Workspaces::handle_message(uint32_t type, unique_ptr<char[]> response)
{
    switch(type)
    {
    case I3_INVALID_TYPE:
        return invalid_event();
    case I3_IPC_EVENT_MODE:
        return mode_event(response.get());
    case I3_IPC_EVENT_WINDOW:
        return window_event(response.get());
    case I3_IPC_EVENT_WORKSPACE:
        return workspace_event(response.get());
    case I3_IPC_EVENT_OUTPUT:
        return output_event(response.get());
    case I3_IPC_REPLY_TYPE_SUBSCRIBE:
        log() << "Subscribed to events - " << response.get() << endl;
        return true;
    default:
        log() << "Unhandled event type: " << ipc_type_to_string(type) << endl;
        return true;
    }
}

void I3Workspaces::subscribe_to_events(void)
{
    string abonnements = "[\"workspace\",\"mode\",\"output\",\"window\"]";
    send_message(
        event_socket, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, move(abonnements));
}

string extract_socket_path(void)
{
    for(int i = 0; i < Application::argc; i++)
    {
        if(string(Application::argv[i]).substr(0, 9) == "--socket=")
        {
            return string(Application::argv[i] + 9);
        }
    }
    return "";
}

I3Workspaces::I3Workspaces(JSON::Node const& item) : StateItem(item)
{
    auto socket_path = extract_socket_path();
    if(socket_path.length() == 0)
    {
        log() << "Socket path not specified in arguments!" << endl;
        return;
    }

    event_socket = init_socket(socket_path);
    register_event_socket(event_socket);
    subscribe_to_events();

    i3.set_command_socket(init_socket(socket_path));
}

bool I3Workspaces::handle_events(int fd)
{
    uint32_t type;
    auto response = read_message(fd, type);
    return handle_message(type, move(response));
}

bool I3Workspaces::update(void)
{
    return true;
}

void I3Workspaces::print(ostream& out, uint8_t display)
{
    i3.print(out, display);
}
