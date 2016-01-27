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

#define GET_DISPLAY_NUM double n = object["current"].object()["num"].number();

void EventHandler::workspace_event(char* response)
{
  JSON* json = JSON::parse(response);
  JSONObject& object = json->object();
  try
  {
    string change = object["change"].string();
    if(change.compare("init") == 0)
    {
      GET_DISPLAY_NUM
      i3State.workspaceInit((uint8_t)n);
    }
    else if(change.compare("focus") == 0)
      i3State.updateWorkspaceStatus();
    else if(change.compare("urgent") == 0)
      i3State.updateWorkspaceStatus();
    else if(change.compare("empty") == 0)
    {
      GET_DISPLAY_NUM
      i3State.workspaceEmpty((uint8_t)n);
    }
    else
      log() << "Unhandled workspace event type: " << change << endl;
  }
  catch(TraceCeption& e)
  {
    delete json;
    throw e;
  }
  delete json;
}

string EventHandler::getWindowName(JSONObject& container)
{
  try
  {
    string name = container["name"].string();
    if(name.length() > 20)
    {
      string class_ = container["window_properties"].object()["class"].string();
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
  JSON* json = JSON::parse(response);
  JSONObject& windowEvent = json->object();
  string change = windowEvent["change"].string();

  JSONObject& container = windowEvent["container"].object();
  long appId = container["id"].number();

  /** New events are also accompanied by focus events if necessary */
  // cLen == 3 && strncmp(cStr, "new", 3) == 0

  /**
   * On a title update check if the application is focused
   * on any visible workspace. If yes -> update (id does not change)
   */
  if(change.compare("title") == 0)
  {
    for(auto i = i3State.workspaces.begin(); i != i3State.workspaces.end(); i++)
      if(i->focusedAppID == appId)
      {
        i->focusedApp = getWindowName(container);
        break;
      }
  }
  /** Copy the title/id of the currently focused window to it's WS. */
  else if(change.compare("focus") == 0)
  {
    Workspace& fw = i3State.workspaces[i3State.focusedWorkspace];
    fw.focusedApp = getWindowName(container);
    fw.focusedAppID = appId;
  }
  /**
   * usually after a close event a focus event is issued,
   * if there is a focused window. therefore delete the focus here.
   * I don't know on which workspace the application closed.
   */
  else if(change.compare("close") == 0)
  {
    for(auto i = i3State.workspaces.begin(); i != i3State.workspaces.end(); i++)
    {
      if(i->focusedAppID == appId)
      {
        i->focusedAppID = -1;
        i->focusedApp = "";
        break;
      }
    }
  }
  else if(change.compare("fullscreen_mode") == 0)
  {
    log() << "Fullscreen mode TODO if anything at all..." << endl;
  }
  else if(change.compare("move") == 0)
  {
    log() << "Move event TODO update titles" << endl;
  }
  else
    log() << "Unhandled window event type: " << change << endl;

  delete json;
  return;
}

void EventHandler::handle_event(uint32_t type, char* response)
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
      i3State.mutex.lock();
      JSON* json = JSON::parse(response);
      JSONObject& modeEvent = json->object();
      i3State.mode = modeEvent["change"].string();
      delete json;
      i3State.mutex.unlock();
    }
    break;
    case I3_IPC_EVENT_WINDOW:
      window_event(response);
      break;
    case I3_IPC_EVENT_WORKSPACE:
      workspace_event(response);
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
    e.push_stack(std::string(response));
    e.push_stack("While handling an event of type: " +
                 ipc_type_to_string(type));
    throw e;
  }

  if(i3State.valid == 0) // is this necessary?
  {
    log() << "Invalid change after event " << type << " occured" << endl;
    if(response != NULL)
      log() << response << endl;
    global.die = 1;
  }

  if(response != NULL)
    free(response);
}

void EventHandler::start(EventHandler* instance) { instance->run(); }

void EventHandler::run(void)
{
  log() << "Forked event handler" << endl;
  char abonnements[] = "[\"workspace\",\"mode\",\"output\",\"window\"]";
  sendMessage(
      push_socket, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, abonnements, global.die);

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
    char* response = readMessage(push_socket, &type, global.die);
    try
    {
      handle_event(type, response);

      while(!global.die && hasInput(push_socket, 1000))
      {
        response = readMessage(push_socket, &type, global.die);
        handle_event(type, response);
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
