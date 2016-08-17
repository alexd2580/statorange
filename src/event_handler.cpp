#include <csignal>
#include <cstring>
#include <iostream>
#include <sys/socket.h>

#include "event_handler.hpp"

#include "JSON/jsonParser.hpp"

#include "i3-ipc-constants.hpp"
#include "i3-ipc.hpp"
#include "i3state.hpp"
#include "util.hpp"

using namespace std;

/******************************************************************************/

EventHandler::EventHandler(I3State& i3, int fd, GlobalData& global_)
    : Logger("[EventHandler]", cerr), i3State(i3), push_socket(fd),
      global(global_)
{
}

void EventHandler::workspace_event(char* response)
{
  auto json_uptr = JSON::parse(response);
  auto& object = *json_uptr;

  string change(object["change"]);
  if(change.compare("init") == 0)
  {
    uint8_t n = object["current"]["num"];
    i3State.workspace_init(n);
  }
  else if(change.compare("focus") == 0)
    i3State.workspace_status();
  else if(change.compare("urgent") == 0)
    i3State.workspace_status();
  else if(change.compare("empty") == 0)
  {
    uint8_t n = object["current"]["num"];
    i3State.workspace_empty(n);
  }
  else
    log() << "Unhandled workspace event type: " << change << endl;
}

string EventHandler::getWindowName(JSON const& container)
{
  try
  {
    string name(container["name"]);
    if(name.length() > 20)
    {
      string class_(container["window_properties"]["class"]);
      if(class_.length() > 20)
      {
        string sub = name.substr(0, 15);
        return sub + "[...]";
      }
      else
        return class_;
    }
    else
      return name;
  }
  catch(TraceCeption& e)
  {
    log() << "Exception when trying to get window name/class" << endl;
    e.printStackTrace();
    return "ERROR";
  }
}

void EventHandler::window_event(char* response)
{
  auto json_uptr = JSON::parse(response);
  auto& wevent = *json_uptr;
  string change = wevent["change"];

  auto& container = wevent["container"];
  long window_id = container["id"];

  /** New events are also accompanied by focus events if necessary */
  // cLen == 3 && strncmp(cStr, "new", 3) == 0

  if(change.compare("new") == 0)
  {
    // cerr << endl << "NEW" << endl << response << endl;
  }

  /**
   * On a title update add application name
   */
  else if(change.compare("title") == 0)
  {
    cerr << endl << "TITLE" << endl << response << endl;
    i3State.window_titles[window_id] = getWindowName(container);
  }
  /** Copy the title/id of the currently focused window to it's WS. */
  else if(change.compare("focus") == 0)
  {
    /*Workspace& fw = i3State.workspaces[i3State.focusedWorkspace];
    fw.focusedApp = getWindowName(container);
    fw.focusedAppID = appId;*/
    // todo notify workspace
  }
  /**
   * usually after a close event a focus event is issued,
   * if there is a focused window. therefore delete the focus here.
   * I don't know on which workspace the application closed.
   */
  else if(change.compare("close") == 0)
  {
    i3State.window_titles.erase(window_id);
    // todo notify workspace
  }
  else if(change.compare("fullscreen_mode") == 0)
  {
    log() << "Fullscreen mode TODO if anything at all..." << endl;
  }
  else if(change.compare("move") == 0)
  {
    log() << "Move event TODO update titles" << endl;
    // cerr << endl << "TITLE" << endl << response << endl;
  }
  else
    log() << "Unhandled window event type: " << change << endl;
}

void EventHandler::handle_event(uint32_t type, std::unique_ptr<char[]> response)
{
  try
  {
    switch(type)
    {
    case I3_INVALID_TYPE:
      if(!global.die)
        log() << "Invalid packet received. Aborting" << endl;
      global.die = 1;
      break;
    case I3_IPC_EVENT_MODE:
    {
      auto json_uptr = JSON::parse(response.get());
      auto& mevent = *json_uptr;
      i3State.mutex.lock();
      i3State.mode.assign(mevent["change"]);
      i3State.mutex.unlock();
    }
    break;
    case I3_IPC_EVENT_WINDOW:
      window_event(response.get());
      break;
    case I3_IPC_EVENT_WORKSPACE:
      workspace_event(response.get());
      break;
    case I3_IPC_EVENT_OUTPUT:
      log() << "Output event - Restarting application" << endl;
      global.exit_status = EXIT_RESTART;
      global.die = 1;
      break;
    default:
      log() << "Unhandled event type: " << ipc_type_to_string(type) << endl;
      break;
    }
  }
  catch(TraceCeption& e)
  {
    e.push_stack(std::string(response.get()));
    e.push_stack("While handling an event of type: " +
                 ipc_type_to_string(type));
    throw e;
  }

  if(i3State.valid == 0) // is this necessary?
  {
    log() << "Invalid change after event " << type << " occured" << endl;
    if(response != NULL)
      log() << response.get() << endl;
    global.die = 1;
  }
}

void EventHandler::start(EventHandler* instance) { instance->run(); }

void EventHandler::run(void)
{
  log() << "Forked event handler" << endl;
  auto thread_id = std::this_thread::get_id();
  log() << "Setting event handler thread id: " << thread_id << endl;
  global.handler_thread_id = thread_id;
  log() << "Setting event handler pthread id: " << pthread_self() << endl;
  global.handler_pthread_id = pthread_self();
  string abonnements = "[\"workspace\",\"mode\",\"output\",\"window\"]";
  send_message(push_socket,
               I3_IPC_MESSAGE_TYPE_SUBSCRIBE,
               global.die,
               move(abonnements));

  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = 10000000;

  log() << "Entering event handler loop" << endl;
  while(!global.die)
  {
    // this sleep prevents the application from dying because of SIGUSR1 spam.
    // on the other hand, the user can now crash, or at least DOS i3 with
    // events, which cannot be processed fast enough
    // could be replaced with a mutex...
    nanosleep(&t, nullptr);

    uint32_t type;
    auto response = read_message(push_socket, &type, global.die);
    try
    {
      handle_event(type, std::move(response));

      while(!global.die && hasInput(push_socket, 1000))
      {
        auto consecutive = read_message(push_socket, &type, global.die);
        handle_event(type, std::move(consecutive));
      }
    }
    catch(TraceCeption& e)
    {
      log() << "Catched TraceCeption" << endl;
      e.printStackTrace();
      // TODO open new socket?
      log() << "Retrying" << endl;
    }
    unique_lock<std::mutex> lock(mutex);
    global.notifier.notify_one();
  }
  log() << "Exiting event handler loop" << endl;
}

void EventHandler::fork(void)
{
  log() << "Forking EventHandler" << endl;
  event_handler_thread = thread(start, this);
}

void EventHandler::join(void)
{
  log() << "Joining EventHandler" << endl;
  if(event_handler_thread.joinable())
  {
    event_handler_thread.join();
    log() << "EventHandler joined" << endl;
  }
  else
    log() << "Cannot join thread" << endl;
}
