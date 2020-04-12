
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <sys/socket.h>

#include "Application.hpp"
#include "i3/i3-ipc-constants.hpp"
#include "i3/i3-ipc.hpp"
#include "json_parser.hpp"
#include "output.hpp"
#include "util.hpp"
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


bool I3Workspaces::output_event(char const*)
{
    log() << "Output event - Restarting application" << endl;
    Application::dead = true;
    Application::exit_status = 1;
    return false;
}


I3Workspaces::I3Workspaces(JSON::Node const& item) : StateItem(item)
{
    auto socket_path = extract_socket_path();
    if(socket_path.length() == 0)
    {
        log() << "Socket path not specified in arguments!" << endl;
        return;
    }

    event_socket = connect_to(socket_path);
    register_event_socket(event_socket);
    subscribe_to_events();

    i3.set_command_socket(connect_to(socket_path));
}
