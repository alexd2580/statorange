/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */

#include <cassert>
#include <csignal>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <fstream>
#include <iostream>

#include <pwd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "json_parser.hpp"

#include "Application.hpp"
#include "StateItem.hpp"
#include "output.hpp"
#include "util.hpp"

using namespace std;

/******************************************************************************/

typedef struct sigaction Sigaction;
Sigaction mk_handler(sighandler_t handler)
{
    Sigaction handler_action;
    handler_action.sa_handler = handler;
    sigemptyset(&handler_action.sa_mask);
    // sa.sa_flags = SA_RESTART | SA_NODEFER;
    handler_action.sa_flags = 0;
    return handler_action;
}

void register_handler(int sig, Sigaction& handler)
{
    sigaction(sig, &handler, nullptr);
}

/**
 * Handler for sigint and sigterm.
 */
void term_handler(int signum)
{
    static Logger term("Term handler");

    term.log() << "Termination requested: " << signum << std::endl;
    Application::dead = true;
}

/**
 * Used to force a systemstate update (sigusr1)
 */
void notify_handler(int signum)
{
    static Logger nlog("Notify handler");

    nlog.log() << "Received notify signal: " << signum << std::endl;
    Application::force_update = true;
}

/******************************************************************************/

JSON::Node load_config(void)
{
    struct passwd* pw = getpwuid(getuid());
    string home_dir(pw->pw_dir);
    string config_name("config.json");
    vector<string> config_paths{"./" + config_name,
                                home_dir + "/.config/statorange/" + config_name,
                                home_dir + ".statorange/" + config_name};

    for(auto const& path : config_paths)
    {
        Logger::log("Config loader") << "Trying config path: " << path << endl;
        string config_string;
        if(load_file(path, config_string))
        {
            try
            {
                return JSON::Node(config_string.c_str());
            }
            catch(char const* err_msg)
            {
                Logger::log("Config path") << "Failed to load config from '"
                                           << path << "':" << std::endl
                                           << err_msg << std::endl;
            }
        }
    }
    return JSON::Node({});
}

// string socket_path(argv[1]);
// cerr << "Socket path: " << socket_path << endl;

// auto& ws_group_json = config_json.get("ws window names");
// auto& ws_group_string = ws_group_json.as_string_with_default("");
// WorkspaceGroup show_names_on(parse_workspace_group(ws_group_string));

int main(int argc, char* argv[])
{
    Application::argc = argc;
    Application::argv = argv;

    LoggerManager::set_stream(cerr);
    Logger l("Main");
    l.log() << "Launching Statorange" << endl;

    auto config_json = load_config();
    if(!config_json.exists())
    {
        l.log() << "Failed to load config." << endl;
        return 1;
    }

    // Init StateItems.
    StateItem::init(config_json);

    // signal handlers and event handlers
    Sigaction term = mk_handler(term_handler);
    register_handler(SIGINT, term);
    register_handler(SIGTERM, term);

    Sigaction notify = mk_handler(notify_handler);
    register_handler(SIGUSR1, notify);

    l.log() << "Entering main loop" << endl;
    while(!Application::dead)
    {
        if(Application::force_update)
        {
            StateItem::force_update_all();
            Application::force_update = false;
        }
        else
            StateItem::update_all();

        for(uint8_t i = 0; i < num_output_displays; i++)
        {
            auto printer = [i](ostream& out) {
                StateItem::print_state(out, i);
            };
            BarWriter::display(cout, i, printer);
        }
        cout << endl;

        StateItem::wait_for_events();
    }
    l.log() << "Exiting main loop" << endl;

    return Application::exit_status;
}
