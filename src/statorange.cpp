/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <iostream>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <fstream>

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
  static Logger term("[Term handler]");

  global->die = 1;

  auto thread_id = std::this_thread::get_id();
  term.log() << "This thread id:          " << thread_id << endl;
  term.log() << "Main thread id:          " << global->main_thread_id << endl;
  term.log() << "Event handler thread id: " << global->handler_thread_id
             << endl;

  if(thread_id == global->main_thread_id)
  {
    term.log() << "main() received signal " << signum << endl;
    term.log() << "Interrupting event_listener()" << endl;
    pthread_kill(global->handler_pthread_id, SIGINT);
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
  static Logger nlog("[Notify handler]");

  global->force_update = 1;
  global->notifier.notify_one();

  auto thread_id = std::this_thread::get_id();
  nlog.log() << "This thread id:          " << thread_id << endl;
  nlog.log() << "Main thread id:          " << global->main_thread_id << endl;
  nlog.log() << "Event handler thread id: " << global->handler_thread_id
             << endl;

  if(thread_id == global->handler_thread_id)
  {
    nlog.log() << "event_listener() received signal " << signum << endl;
    nlog.log() << "Interrupting main()" << endl;
    pthread_kill(global->main_pthread_id, SIGUSR1);
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

  fstream log_file;
  log_file.open("log/statorange.log", fstream::out);
  LoggerManager::set_stream(log_file);

  Logger l("[Main]");
  l.log() << "Launching Statorange" << endl;

  if(argc < 3)
  {
    cout << "Please supply the socket and config paths." << endl;
    return EXIT_FAILURE;
  }

  GlobalData global_data;
  global_data.die = false;
  global = &global_data;

  auto thread_id = std::this_thread::get_id();
  l.log() << "Setting main thread id: " << thread_id << endl;
  global_data.main_thread_id = thread_id;
  l.log() << "Setting main pthread id: " << pthread_self() << endl;
  global_data.main_pthread_id = pthread_self();

  string socket_path(argv[1]);
  l.log() << "Socket path: " << socket_path << endl;
  string config_path(argv[2]);
  l.log() << "Config path: " << config_path << endl;

  string config_string;
  if(!load_file(config_path, config_string))
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

  global_data.die = true;
  i3State.mutex.unlock(); // returns error if already unlocked (ignore)
  lock.unlock();
  l.log() << "Exiting main loop" << endl;
  close(push_socket);
  event_handler.join();

  StateItem::deinit();

  l.log() << "Stopping Statorange" << endl;
  cout.flush();
  cerr.flush();
  log_file.flush();
  log_file.close();

  return global_data.exit_status;
}
