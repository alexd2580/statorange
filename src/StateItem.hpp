#ifndef __STATEITEMHEADER_LOL___
#define __STATEITEMHEADER_LOL___

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "JSON/json_parser.hpp"
#include "Logger.hpp"

class StateItem : public Logger
{
  private:
    static std::chrono::seconds min_cooldown;
    static std::vector<StateItem*> states;
    static std::map<int, StateItem*> event_sockets;

    std::string module_name;
    std::chrono::seconds cooldown;
    std::chrono::system_clock::time_point last_updated;
    bool button;
    std::string button_command;
    bool valid;

    void wrap_update(void);
    void force_update(void);
    void wrap_print(void);

  protected:
    StateItem(std::string const&, JSON const&);

    void register_event_socket(int fd);
    virtual void handle_events(void);
    void unregister_event_socket(int fd) const;

    virtual bool update(void) = 0;
    virtual void print(void) = 0;

  public:
    virtual ~StateItem(void) = default;

    // This method initializes the settings for each type of item.
    static void init(JSON const& config);
    static void update_all(void);
    static void force_update_all(void);
    static void print_state(void);
    static void deinit(void);

    static void wait_for_events(void);
};

#endif
