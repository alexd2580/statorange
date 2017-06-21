#ifndef __STATEITEMHEADER_LOL___
#define __STATEITEMHEADER_LOL___

#include <chrono>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "JSON/json_parser.hpp"
#include "Logger.hpp"
#include "output.hpp"

class StateItem : public Logger
{
  private:
    static std::chrono::seconds min_cooldown;
    static std::vector<std::unique_ptr<StateItem>> left_items;
    static std::vector<std::unique_ptr<StateItem>> center_items;
    static std::vector<std::unique_ptr<StateItem>> right_items;
    static std::map<int, StateItem*> event_sockets;

    std::chrono::system_clock::time_point last_updated;
    bool valid;

    bool cached;
    std::string print_string;

    void wrap_update(void);
    void force_update(void);
    void wrap_print(std::ostream&, uint8_t);

  protected:
    std::string const module_name;
    std::chrono::seconds const cooldown;
    // The icon is a core component of the config. However, it is not printed
    // automatically and nees to be printed manually.
    BarWriter::Icon const icon;
    bool const button;
    std::string const button_command;

    StateItem(JSON const&);

    void register_event_socket(int fd);
    virtual void handle_events(void);
    void unregister_event_socket(int fd) const;

    virtual bool update(void) = 0;
    virtual void print(std::ostream&, uint8_t) = 0;

  public:
    virtual ~StateItem(void) = default;

  private:
    static StateItem* init_item(JSON const& json_item);
    static void init_section(
        JSON const& config,
        std::string const& section_name,
        std::vector<std::unique_ptr<StateItem>>& section);

  public:
    // This method initializes the settings for each type of item.
    static void init(JSON const& config);
    static void update_all(void);
    static void force_update_all(void);

  private:
    static void print_section(
        std::ostream& out,
        BarWriter::Alignment a,
        std::vector<std::unique_ptr<StateItem>> const& section,
        uint8_t display_number);

  public:
    static void print_state(std::ostream&, uint8_t);

    static void wait_for_events(void);
};

#endif
