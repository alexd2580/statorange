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
  /**
   * Long story short - C++ std::threads do not support killing each other. LOL
   * This hack works as long as
   *  std::thread::native_handle_type == __gthread == pthread_t
   * if it fails, it should do so at compile-time.
   */
  std::thread::id main_thread_id;
  pthread_t main_pthread_id;
  std::thread::id handler_thread_id;
  pthread_t handler_pthread_id;

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
  std::string getWindowName(JSON const& container);

  void workspace_event(char* response);
  void window_event(char* response);

  /**
   * Handles the incoming event.
   * Assumes ownership of the response string.
   */
  void handle_event(uint32_t type, std::unique_ptr<char[]> response);

  static void start(EventHandler* instance);
  void run(void);

public:
  EventHandler(I3State& i3State, int fd, GlobalData& global);
  virtual ~EventHandler(void) = default;

  void fork(void);
  void join(void);
};

#endif
