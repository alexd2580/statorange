#ifndef STATEITEM_HPP
#define STATEITEM_HPP

#include <chrono>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "Lemonbar.hpp"
#include "Logger.hpp"
#include "json_parser.hpp"

class StateItem : public Logger {
  private:
    // The min cooldown is computed as the minimum of all item cooldowns.
    static std::chrono::seconds min_cooldown;

    static std::map<int, StateItem*> event_sockets;

    // Name of this module.
    std::string const module_name;

    // The cooldown of this module.
    std::chrono::seconds const cooldown;

    // Is this module a button?
    bool const button;
    // The command string of the button (if `button == true`).
    std::string const button_command;

    // Set in the base class methods on update.
    std::chrono::system_clock::time_point last_updated;

    // Display a notice if this module's update method failed (valid is set to false).
    bool const show_failed;

    // Set in the base class methods from the result of the specialized classes' `update` result.
    bool valid;
    bool changed;

  protected:
    // The icon is a core component of the config. However, it is not printed automatically.
    Lemonbar::Icon const icon;

    static Lemonbar::Icon parse_icon_from_json(JSON::Node const&);

    explicit StateItem(JSON::Node const&);

    void register_event_socket(int fd);
    void unregister_event_socket(int fd) const;

    // First bool describes whether `update` has succeeded.
    // Second bool describes whether `update` has registered a state-change.
    virtual std::pair<bool, bool> update_raw() = 0;
    virtual std::pair<bool, bool> handle_stream_data_raw(int fd);
    virtual void print_raw(Lemonbar&, uint8_t) = 0;

    bool handle_stream_data(int fd);

  public:
    StateItem(const StateItem&) = delete;
    StateItem(StateItem&&) = delete;
    StateItem& operator=(const StateItem&) = delete;
    StateItem& operator=(StateItem&&) = delete;

    ~StateItem() override = default;

    // Result is true if the state item needs to be redrawn.
    bool update(bool force = false);
    void print(Lemonbar&, uint8_t);

    static bool wait_for_events(int signal_fd = 0);
};

#endif
