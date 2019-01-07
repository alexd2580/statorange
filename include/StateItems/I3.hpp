#ifndef STATEITEMS_I3_HPP
#define STATEITEMS_I3_HPP

#include <ostream>
#include <string>
#include <utility>

#include <fmt/format.h>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"
#include "utils/io.hpp"

class Outputs final {
    // Map of position of a monitor to its Xorg name: LVDS, eDP1, VGA1, HDMI2, DVI3.
    std::map<uint8_t, std::string> outputs;
};

struct Workspace final {
    // Display on which this workspace is located.
    uint8_t display;

    // Name of workspace as defined in `~/.config/i3/config`.
    std::string const name;

    // Urgent flag set by X.
    bool urgent = false;

    Workspace(uint8_t new_display, std::string const& new_name, bool new_urgent)
        : display(new_display), name(new_name), urgent(new_urgent) {}
};

class Workspaces final {
    // Map of unique number to workspace.
    std::map<uint8_t, Workspace> workspaces;
    uint8_t focused = 255;

  public:
    void print_raw(Lemonbar& bar, uint8_t display) {
        for(auto const& workspace_entry : workspaces) {
            auto const& workspace = workspace_entry.second;
            if(workspace.display == display) {
                auto const& sep_right = Lemonbar::Separator::right;
                bool is_focused = workspace_entry.first == focused;
                auto const& coloring = workspace.urgent
                                           ? Lemonbar::Coloring::urgent
                                           : is_focused ? Lemonbar::Coloring::active : Lemonbar::Coloring::inactive;
                bar.separator(sep_right, coloring);
                bar() << " " << workspace.name << " ";
                bar.separator(sep_right, Lemonbar::Coloring::white_on_black);
            }
        }
    }

    void init(uint8_t num, std::string const& name) {
        workspaces.emplace(std::piecewise_construct, std::forward_as_tuple(num),
                           std::forward_as_tuple(0x0, name, false));
    }
    void focus(uint8_t num) { focused = num; }
    void empty(uint8_t num) { workspaces.erase(num); }
};

struct Window final {
    // Workspace on which this window is located.
    uint64_t workspace;

    // Name f the window.
    std::string name;
};

class Windows final {
    // Map of uuid to window.
    std::map<uint64_t, Window> windows;
};

class I3 final : public StateItem {
  private:
    UniqueSocket command_socket;
    UniqueSocket event_socket;

    // i3-mode, defined in the i3-config.
    std::string mode;

    Outputs outputs;
    Workspaces workspaces;
    Windows windows;

    void query_tree();

    void workspace_event(std::unique_ptr<char[]> response);
    void window_event(std::unique_ptr<char[]> response);
    void mode_event(std::unique_ptr<char[]> response);

    std::pair<bool, bool> handle_message(uint32_t type, std::unique_ptr<char[]> response);

    std::pair<bool, bool> update_raw() override;
    std::pair<bool, bool> handle_stream_data_raw(int fd) override;
    void print_raw(Lemonbar&, uint8_t) override;

  public:
    explicit I3(JSON::Node const& item);

    I3(I3 const& other) = delete;
    I3& operator=(I3 const& other) = delete;
    I3(I3&& other) = delete;
    I3& operator=(I3&& other) = delete;

    ~I3() override;
};

#endif
