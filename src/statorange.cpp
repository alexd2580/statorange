/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */
//#define _POSIX_C_SOURCE 200809L

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <thread>

#include <sys/socket.h>
#include <unistd.h>

#include "JSON/jsonParser.hpp"
#include "JSON/JSONException.hpp"

#include "StateItem.hpp"
#include "event_handler.hpp"
#include "i3-ipc.hpp"
#include "i3state.hpp"
#include "output.hpp"
#include "util.hpp"

using namespace std;

/******************************************************************************/
/********************************** GLOBALS ***********************************/

/**
 * Long story short - C++ std::threads do not support killing each other. LOL
 * This hack works as long as
 *  std::thread::native_handle_type == __gthread == pthread_t
 * if it fails, it should do so at compile-time.
 */
static pthread_t main_thread_id;
static pthread_t handler_thread_id;

static GlobalData* global;

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
  static Logger term("[Term handler]", cerr);

  global->die = 1;

  pthread_t this_id = pthread_self();
  if(this_id == main_thread_id)
  {
    term.log() << "main() received signal " << signum << endl;
    term.log() << "Interrupting event_listener()" << endl;
    pthread_kill(handler_thread_id, SIGINT);
  }
  else
  {
    term.log() << "event_listener() received signal " << signum << endl;
    term.log() << "Notifying main()" << endl;
    global->notifier.notify_one();
  }
  return;
}

/**
 * Used to force a systemstate update (sigusr1)
 */
void notify_handler(int signum)
{
  static Logger nlog("[Notify handler]", cerr);

  global->force_update = 1;
  global->notifier.notify_one();

  pthread_t this_id = pthread_self();
  if(this_id == handler_thread_id)
  {
    nlog.log() << "event_listener() received signal " << signum << endl;
    nlog.log() << "Interrupting main()" << endl;
    pthread_kill(main_thread_id, SIGUSR1);
  }
  else
  {
    nlog.log() << "main() received signal " << signum << endl;
    nlog.log() << "Forcing update" << endl;
  }
  return;
}

/******************************************************************************/

int main(int argc, char* argv[])
{
  Logger l("[Main]", cerr);
  l.log() << "Launching Statorange" << endl;

  if(argc < 3)
  {
    cout << "Please supply the socket and config paths." << endl;
    return EXIT_FAILURE;
  }

  GlobalData global_data;
  global_data.die = false;
  global = &global_data;

  string socket_path(argv[1]);
  string config_name(argv[2]);

  string config_string;
  if(!load_file(config_name, config_string))
  {
    l.log() << "Could not load config.json" << endl;
    return EXIT_FAILURE;
  }

  chrono::seconds cooldown(5);
  WorkspaceGroup show_names_on(WorkspaceGroup::visible);
  bool show_failed_modules(true);

  try
  {
    /** Load the config */
    auto config_json_raw = JSON::parse(config_string.c_str());
    auto& config_json = *config_json_raw;

    // cooldown
    auto oldval(cooldown);
    try
    {
      cooldown = chrono::seconds((int)config_json["cooldown"]);
    }
    catch(JSONException&)
    {
      assert(oldval == cooldown);
      // ignore
    }

    try
    {
      auto& sno = config_json["ws window names"];
      show_names_on = parse_workspace_group(sno);
    }
    catch(JSONException&)
    {
      // ignore
    }

    try
    {
      show_failed_modules = config_json["show failed modules"];
    }
    catch(JSONException&)
    {
      // ignore
    }
    // init StateItems
    StateItem::init(config_json);
  }
  catch(TraceCeption& e)
  {
    l.log() << "Could not parse config:" << endl;
    e.printStackTrace();
    return EXIT_FAILURE;
  }

  // init i3State
  I3State i3State(socket_path, global_data.die);
  i3State.init_layout();

  // signal handlers and event handlers
  Sigaction term = mk_handler(term_handler);
  register_handler(SIGINT, term);
  register_handler(SIGTERM, term);

  Sigaction notify = mk_handler(notify_handler);
  register_handler(SIGUSR1, notify);

  int push_socket = init_socket(socket_path);
  EventHandler event_handler(i3State, push_socket, global_data);
  event_handler.fork();

  // main loop
  l.log() << "Entering main loop" << endl;
  std::unique_lock<std::mutex> lock(global_data.mutex); // TODO
  try
  {

    while(!global_data.die) // <- volatile
    {
      if(global_data.force_update)
      {
        StateItem::forceUpdates();
        global_data.force_update = 0;
      }
      else
        StateItem::updates();

      i3State.mutex.lock();
      if(!i3State.valid)
        break;
      echo_lemon(i3State, show_names_on);
      i3State.mutex.unlock();
      global_data.notifier.wait_for(lock, cooldown);
    }
  }
  catch(TraceCeption& e)
  {
    l.log() << "Exception catched:" << endl;
    e.printStackTrace();
    global_data.exit_status = EXIT_FAILURE;
  }

  global_data.die = 1;
  i3State.mutex.unlock(); // returns error if already unlocked (ignore)
  lock.unlock();
  l.log() << "Exiting main loop" << endl;

  close(push_socket);
  event_handler.join();

  StateItem::deinit();

  l.log() << "Stopping Statorange" << endl;
  return global_data.exit_status;
}
