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

#include <sys/socket.h>
#include <unistd.h>

#include "JSON/json_parser.hpp"

#include "Application.hpp"
#include "StateItem.hpp"
#include "i3/I3State.hpp"
#include "i3/i3-ipc.hpp"
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
    static Logger term("[Term handler]");

    Application::dead = true;

    return;
}

/**
 * Used to force a systemstate update (sigusr1)
 */
void notify_handler(int signum)
{
    static Logger nlog("[Notify handler]");

    Application::force_update = true;

    // Forced update
    return;
}

/******************************************************************************/

unique_ptr<JSON> load_config(string const& config_path)
{
    cerr << "Config path: " << config_path << endl;
    string config_string;
    if(!load_file(config_path, config_string))
    {
        cerr << "Could not load the config from " << config_path << endl;
        return {};
    }

    char const* config_cstring = config_string.c_str();
    return JSON::parse(config_cstring);
}

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        cout << "Please supply the socket and config paths." << endl;
        return EXIT_FAILURE;
    }

    string socket_path(argv[1]);
    cerr << "Socket path: " << socket_path << endl;

    auto config_json_raw = load_config(string(argv[2]));
    auto& config_json = *config_json_raw;

    auto& log_path_json = config_json.get("log file path");
    string log_path(log_path_json.as_string_with_default("/dev/null"));

    auto& ws_group_json = config_json.get("ws window names");
    auto& ws_group_string = ws_group_json.as_string_with_default("");
    WorkspaceGroup show_names_on(parse_workspace_group(ws_group_string));

    auto& show_failed_json = config_json.get("show failed modules");
    bool show_failed_modules(show_failed_json.as_bool_with_default(true));

    // Set the log file output.
    fstream log_file;
    log_file.open(log_path, fstream::out);
    LoggerManager::set_stream(log_file);

    Logger l("[Main]");
    l.log() << "Launching Statorange" << endl;

    // Init StateItems.
    StateItem::init(config_json);

    I3State i3State(socket_path);
    i3State.init_layout();

    // signal handlers and event handlers
    Sigaction term = mk_handler(term_handler);
    register_handler(SIGINT, term);
    register_handler(SIGTERM, term);

    Sigaction notify = mk_handler(notify_handler);
    register_handler(SIGUSR1, notify);

    int command_socket = init_socket(socket_path);

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

        if(!i3State.valid)
            break;

        echo_lemon(i3State, show_names_on);

        StateItem::wait_for_events();
    }

    l.log() << "Exiting main loop" << endl;
    close(command_socket);

    StateItem::deinit();

    l.log() << "Stopping Statorange" << endl;
    cout.flush();
    cerr.flush();
    log_file.flush();
    log_file.close();

    return Application::exit_status;
}
