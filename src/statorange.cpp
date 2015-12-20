/**
 * ONLY FOR PRIVATE USE!
 * NEITHER VALIDATED, NOR EXCESSIVELY TESTED
 */

#define _POSIX_C_SOURCE 200809L

#include<cstdlib>
#include<cstring>
#include<iostream>
#include<ctime>
#include<csignal>

#include<unistd.h>
#include<pthread.h>
#include<sys/socket.h>

#include"JSON/jsonParser.hpp"

#include"i3state.hpp"
#include"util.hpp"
#include"output.hpp"
#include"StateItem.hpp"

using namespace std;

/******************************************************************************/
/********************************** GLOBALS ***********************************/

pthread_t main_thread;
pthread_t event_listener_thread;
pthread_cond_t notifier;
pthread_mutex_t mutex;

volatile sig_atomic_t die = 0;
volatile sig_atomic_t exit_status = EXIT_SUCCESS;
volatile sig_atomic_t force_update = 1;

void forkEventListener(I3State* i3, string& path);

/******************************************************************************/

Logger term("[Term handler]", cerr);
void term_handler(int signum)
{
  die = 1;

  pthread_t this_ = pthread_self();
  if(this_ == main_thread)
  {
    term.log() << "main() received signal " << signum << endl;
    term.log() << "Interrupting event_listener()" << endl;
    pthread_kill(event_listener_thread, SIGINT);
  }
  else
  {
    term.log() << "event_listener() received signal " << signum << endl;
    term.log() << "Notifying main()" << endl;
    pthread_cond_signal(&notifier);
  }
  return;
}

/**
 * Used to force a systemstate update
 */
Logger nlog("[Notify handler]", cerr);
void notify_handler(int signum)
{
  force_update = 1;
  pthread_cond_signal(&notifier);

  pthread_t this_ = pthread_self();
  if(this_ == event_listener_thread)
  {
    nlog.log() << "event_listener() received signal " << signum << endl;
    nlog.log() << "Interrupting main()" << endl;
    pthread_kill(main_thread, SIGUSR1);
  }
  else
  {
    nlog.log() << "main() received signal " << signum << endl;
    nlog.log() << "Forcing update" << endl;
  }
  return;
}

void register_signal_handlers(void)
{
  struct sigaction term_handler_action;
  term_handler_action.sa_handler = term_handler;
  sigemptyset(&term_handler_action.sa_mask);
  //sa.sa_flags = SA_RESTART | SA_NODEFER;
  term_handler_action.sa_flags = 0;
  sigaction(SIGINT, &term_handler_action, nullptr);
  sigaction(SIGTERM, &term_handler_action, nullptr);

  struct sigaction sigusr1_handler_action;
  sigusr1_handler_action.sa_handler = notify_handler;
  sigemptyset(&sigusr1_handler_action.sa_mask);
  //sa.sa_flags = SA_RESTART | SA_NODEFER;
  sigusr1_handler_action.sa_flags = 0;
  sigaction(SIGUSR1, &sigusr1_handler_action, nullptr);
}

/******************************************************************************/

int main(int argc, char* argv[])
{
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

  try
  {
    /** Load the config */
    JSON* config_json_raw = JSON::parse(config_string.c_str());
    JSONObject& config_json = config_json_raw->object();

    //cooldown
    time_t cooldown = config_json["cooldown"].number();

    //get the socket path to i3
    string get_socket = config_json["get_socket"].string();
    string path = execute(get_socket);
    path.pop_back();

    //init i3State
    I3State i3State(path);
    i3State.updateOutputs();

    //init StateItems
    StateItem::init(config_json);

    //Now the config can be deleted
    delete config_json_raw;

    /** Initialize and fork event listener */
    pthread_cond_init(&notifier, nullptr);
    pthread_mutex_init(&mutex, nullptr);

    forkEventListener(&i3State, path);
    main_thread = pthread_self();

    register_signal_handlers();

    l.log() << "Entering main loop" << endl;
    pthread_mutex_lock(&mutex);

    try
    {

      while(!die)
      {
        if(force_update)
        {
          StateItem::forceUpdates();
          force_update = 0;
        }
        else
          StateItem::updates();

        pthread_mutex_lock(&i3State.mutex);
        if(!i3State.valid)
          die = 1;
        else
        {
          echoPrimaryLemon(i3State, 0);
          for(uint8_t i=1; i<i3State.outputs.size(); i++)
            echoSecondaryLemon(i3State, i);
          cout << endl;
          cout.flush();
        }
        pthread_mutex_unlock(&i3State.mutex);

        if(die)
          break; //skip time delay here -> annoying
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += cooldown;
        pthread_cond_timedwait(&notifier, &mutex, &abstime);
      }
    }
    catch(TraceCeption& e)
    {
      l.log() << "Exception catched:" << endl;
      e.printStackTrace();
      exit_status = EXIT_FAILURE;
    }
  }
  catch(TraceCeption& e)
  {
    l.log() << "Could not parse config:" << endl;
    e.printStackTrace();
    exit_status = EXIT_FAILURE;
  }

  pthread_mutex_unlock(&mutex);
  l.log() << "Exiting main loop" << endl;

  pthread_join(event_listener_thread, nullptr);

  StateItem::close();

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&notifier);

  l.log() << "Stopping Statorange" << endl;
  return exit_status;
}
