/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */
//#define _POSIX_C_SOURCE 200809L

#include <csignal>
#include <cstdlib>
#include <cstring>
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
  GlobalData global_data;
  global = &global_data;

  Logger l("[Main]", cerr);
  l.log() << "Launching Statorange" << endl;

  if(argc < 2)
  {
    cout << "Please supply the config path." << endl;
    return EXIT_FAILURE;
  }

  string config_name(argv[1]);
  string config_string;
  if(!load_file(config_name, config_string))
  {
    l.log() << "Could not load config.json" << endl;
    return EXIT_FAILURE;
  }

  init_colors();

  chrono::seconds cooldown;
  string path;

  try
  {
    /** Load the config */
    JSON* config_json_raw = JSON::parse(config_string.c_str());
    JSONObject& config_json = config_json_raw->object();

    // cooldown
    int cooldown_int = config_json["cooldown"].number();
    cooldown = chrono::seconds(cooldown_int);

    // get the socket path to i3
    string get_socket = config_json["get_socket"].string();
    if(!execute(get_socket, path, global_data.die))
    {
      l.log() << "Could not get i3 socket path" << endl;
      delete config_json_raw;
      return EXIT_FAILURE;
    }
    l.log() << "Socket path: " << path << endl;
    path.pop_back();

    // init StateItems
    StateItem::init(config_json);

    // Now the config can be deleted
    delete config_json_raw;
  }
  catch(TraceCeption& e)
  {
    l.log() << "Could not parse config:" << endl;
    e.printStackTrace();
    return EXIT_FAILURE;
  }

  // init i3State
  I3State i3State(path, global_data.die);
  i3State.updateOutputs();

  // signal handlers and event handlers
  Sigaction term = mk_handler(term_handler);
  register_handler(SIGINT, term);
  register_handler(SIGTERM, term);

  Sigaction notify = mk_handler(notify_handler);
  register_handler(SIGUSR1, notify);

  int push_socket = init_socket(path.c_str());
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
      echo_lemon(i3State);
      i3State.mutex.unlock();
      global_data.notifier.wait_for(lock, std::chrono::seconds(5));
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

  shutdown(push_socket, SHUT_RDWR);
  event_handler.join();

  StateItem::close();

  l.log() << "Stopping Statorange" << endl;
  return global_data.exit_status;
}
