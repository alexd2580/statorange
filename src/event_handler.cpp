#include <csignal>
#include <cstring>
#include <iostream>
#include <sys/socket.h>

#include "event_handler.hpp"

#include "i3-ipc-constants.hpp"
#include "i3-ipc.hpp"
#include "i3state.hpp"
#include "util.hpp"

using namespace std;

/******************************************************************************/

EventHandler::EventHandler(I3State& i3, int fd, GlobalData& global_)
    : Logger("[EventHandler]"), i3State(i3), push_socket(fd), global(global_)
{
}

string EventHandler::get_window_name(JSON const& container)
{
  std::string default_value("ERROR");
  string name(container.get("name").as_string_with_default(default_value));
  log() << "Requesting window name: " << name << endl;

  if(name.length() > 20)
  {
    string class_(container.get("window_properties")
                      .get("class")
                      .as_string_with_default(default_value));
    log() << "Too long window name, using class: " << class_ << endl;
    if(class_.length() > 20)
    {
      string sub = name.substr(0, 17);
      return sub + "[…]";
    }
    return class_;
  }
  return name;
}

void EventHandler::invalid_event(void)
{
  if(!global.die)
    log() << "Invalid packet received. Aborting" << endl;
  global.die = true;
}

void EventHandler::mode_event(char const* response)
{
  auto json_uptr = JSON::parse(response);
  auto& mevent = *json_uptr;
  i3State.mutex.lock();
  i3State.mode.assign(mevent["change"]);
  i3State.mutex.unlock();
}

void EventHandler::window_event(char const* response)
{
  auto json_uptr = JSON::parse(response);
  auto& wevent = *json_uptr;
  string change = wevent["change"];

  auto& container = wevent["container"];
  long window_id = container["id"];

  /** New events are also accompanied by focus events if necessary */
  if(change.compare("new") == 0)
  {
    auto focused_output = i3State.focused_output;
    // log() << "Focused output " << focused_output << ' ' <<
    // focused_output->name
    //      << endl;
    auto focused_workspace = focused_output->focused_workspace;
    // log() << "Focused workspace " << focused_workspace->name << endl;

    auto name = get_window_name(container);
    i3State.windows[window_id] = {window_id, name, focused_workspace};
  }
  else if(change.compare("title") == 0)
  {
    i3State.windows[window_id].name = get_window_name(container);
  }
  /** Copy the title/id of the currently focused window to it's WS. */
  else if(change.compare("focus") == 0)
  {
    Output& focused_output = *i3State.focused_output;
    Workspace& focused_workspace = *focused_output.focused_workspace;
    focused_workspace.focused_window_id = window_id;

    if(i3State.windows.find(window_id) == i3State.windows.end())
      i3State.windows[window_id].name = get_window_name(container);
  }
  /**
   * usually after a close event a focus event is issued,
   * if there is a focused window. therefore delete the focus here.
   * I don't know on which workspace the application closed.
   */
  else if(change.compare("close") == 0)
  {
    i3State.windows.erase(window_id);
  }
  else if(change.compare("fullscreen_mode") == 0)
  {
    log() << "Ignoring fullscreen mode event" << endl;
  }
  else if(change.compare("move") == 0)
  {
    log() << "Ignoring move event" << endl;
  }
  else
    log() << "Unhandled window event type: " << change << endl;
}

void EventHandler::workspace_event(char const* response)
{
  auto json_uptr = JSON::parse(response);
  auto& object = *json_uptr;

  string change(object["change"]);
  auto& current = object["current"];
  if(change.compare("init") == 0)
    i3State.workspace_init(current);
  else if(change.compare("focus") == 0)
    i3State.workspace_focus(current);
  else if(change.compare("urgent") == 0)
    i3State.workspace_urgent(current);
  else if(change.compare("empty") == 0)
    i3State.workspace_empty(current);
  else
    log() << "Unhandled workspace event type: " << change << endl;
}

void EventHandler::output_event(char const*)
{
  log() << "Output event - Restarting application" << endl;
  global.exit_status = EXIT_RESTART;
  global.die = true;
}

void EventHandler::handle_event(uint32_t type, std::unique_ptr<char[]> response)
{
  switch(type)
  {
  case I3_INVALID_TYPE:
    invalid_event();
    break;
  case I3_IPC_EVENT_MODE:
    mode_event(response.get());
    break;
  case I3_IPC_EVENT_WINDOW:
    window_event(response.get());
    break;
  case I3_IPC_EVENT_WORKSPACE:
    workspace_event(response.get());
    break;
  case I3_IPC_EVENT_OUTPUT:
    output_event(response.get());
    break;
  case I3_IPC_REPLY_TYPE_SUBSCRIBE:
    log() << "Subscribed to events - " << response.get() << endl;
    break;
  default:
    log() << "Unhandled event type: " << ipc_type_to_string(type) << endl;
    break;
  }

  if(!i3State.valid) // is this necessary?
  {
    log() << "Invalid change after event of type " << ipc_type_to_string(type)
          << " occured." << endl;
    if(response)
      log() << response.get() << endl;
    global.die = true;
  }
}

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
    handle_event(type, std::move(response));

    while(!global.die && hasInput(push_socket, 1000))
    {
      auto consecutive = read_message(push_socket, &type, global.die);
      handle_event(type, std::move(consecutive));
    }

    unique_lock<std::mutex> lock(global.mutex);
    global.notifier.notify_one();
  }
  log() << "Exiting event handler loop" << endl;
}

void EventHandler::start(EventHandler* instance) { instance->run(); }

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
