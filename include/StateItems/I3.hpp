#ifndef STATEITEMS_I3_HPP
#define STATEITEMS_I3_HPP

#include <ostream>
#include <string>
#include <utility>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"

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
};

class Workspaces final {
    // Map of unique number to workspace.
    std::map<uint8_t, Workspace> workspaces;

  public:
    void print_raw(Lemonbar& bar, uint8_t display) {
        for(auto const& workspace_entry : workspaces) {
            auto const& workspace = workspace_entry.second;
            if (workspace.display == display) {
                bar.separator(Lemonbar::Separator::right, Lemonbar::Coloring::neutral);
                bar() << " " << workspace.name << " ";
                bar.separator(Lemonbar::Separator::right, Lemonbar::Coloring::white_on_black);
            }
        }
    }
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
    int command_socket;
    int event_socket;

    // i3-mode, defined in the i3-config.
    std::string mode;

    Outputs outputs;
    Workspaces workspaces;
    Windows windows;

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
