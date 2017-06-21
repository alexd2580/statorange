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

#include "JSON/json_parser.hpp"

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

    Application::dead = true;

    return;
}

/**
 * Used to force a systemstate update (sigusr1)
 */
void notify_handler(int signum)
{
    static Logger nlog("Notify handler");

    Application::force_update = true;

    // Forced update
    return;
}

/******************************************************************************/

unique_ptr<JSON> load_config(void)
{
    struct passwd* pw = getpwuid(getuid());
    string home_dir(pw->pw_dir);
    string config_name("config.json");
    vector<string> config_paths{"./" + config_name,
                                home_dir + "/.config/statorange/" + config_name,
                                home_dir + ".statorange/" + config_name};

    for(auto const& path : config_paths)
    {
        cerr << "Trying config path: " << path << endl;
        string config_string;
        if(load_file(path, config_string))
        {
            char const* config_cstring = config_string.c_str();
            return JSON::parse(config_cstring);
        }
    }
    return {};
}

// string socket_path(argv[1]);
// cerr << "Socket path: " << socket_path << endl;

// auto& ws_group_json = config_json.get("ws window names");
// auto& ws_group_string = ws_group_json.as_string_with_default("");
// WorkspaceGroup show_names_on(parse_workspace_group(ws_group_string));

// auto& show_failed_json = config_json.get("show failed modules");
// bool show_failed_modules(show_failed_json.as_bool_with_default(true));

int main(int, char* [])
{
    LoggerManager::set_stream(cerr);
    Logger l("Main");
    l.log() << "Launching Statorange" << endl;

    auto config_json_raw = load_config();
    if(!config_json_raw)
    {
        l.log() << "Failed to load config." << endl;
        return 1;
    }
    auto& config_json = *config_json_raw;

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
            auto printer = [&i](ostream& out) {
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
