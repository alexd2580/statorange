#ifndef __EVENTHANDLER_HEADER_RAMEN_
#define __EVENTHANDLER_HEADER_RAMEN_

#include <condition_variable>
#include <mutex>
#include <thread>

#include "JSON/jsonParser.hpp"
#include "i3state.hpp"
#include "util.hpp"

struct GlobalData
{
  std::condition_variable notifier;
  std::mutex mutex;

  bool die;
  int exit_status;
  bool force_update;
};

class EventHandler : public Logger
{
public:
  std::thread event_handler_thread;

private:
  I3State& i3State;
  int const push_socket;
  GlobalData& global;

  /* ??
      extern volatile sig_atomic_t exit_status;
  */
  std::string getWindowName(JSONObject& container);

  void workspace_event(char* response);
  void window_event(char* response);
  /**
   * Handles the incoming event. response is NOT freed.
   */
  void handle_event(uint32_t type, char* response);

  static void start(EventHandler* instance);
  void run(void);

public:
  EventHandler(I3State& i3State, int fd, GlobalData& global);
  ~EventHandler(void) {}

  void fork(void);
  void join(void);
};

#endif
