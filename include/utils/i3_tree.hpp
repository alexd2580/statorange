#ifndef UTILS_I3_TREE_HPP
#define UTILS_I3_TREE_HPP

#include <algorithm>
#include <map>
#include <string>

#include "Lemonbar.hpp"

struct Output final {
    std::string const name;

    // Urgent flag set by X.
    uint8_t focused_workspace = 255;
};

// Map of position of a monitor to its Xorg name: LVDS, eDP1, VGA1, HDMI2, DVI3.
class Outputs final : private std::map<uint8_t, Output> {
  private:
    Outputs::const_iterator get_output(std::string const& name) const {
        auto is_searched_display = [&](auto const& pair) { return pair.second.name == name; };
        return std::find_if(cbegin(), cend(), is_searched_display);
    }

  public:
    // Get the display number for a display name. If no such display is present,
    // then return 255.
    uint8_t get_num(std::string const& name) const {
        auto display_iter = get_output(name);
        return display_iter == cend() ? 255 : display_iter->first;
    }

    uint8_t get_workspace_num_of_focused_on(std::string const& name) {
        auto display_iter = get_output(name);
        return display_iter == cend() ? 255 : display_iter->second.focused_workspace;
    }

    void add(uint8_t num, std::string const& name) { emplace(num, Output{name, 255}); }
    void focus_workspace(uint8_t num, uint8_t workspace) { at(num).focused_workspace = workspace; }
    void clear() { std::map<uint8_t, Output>::clear(); }
};

struct Workspace final {
    // Display on which this workspace is located.
    uint8_t display;

    // Name of workspace as defined in `~/.config/i3/config`.
    std::string const name;

    // Urgent flag set by X.
    bool urgent = false;

    uint64_t focused_window;
};

// Map of unique number to workspace.
class Workspaces final : private std::map<uint8_t, Workspace> {
  public:
    // Focused workspace.
    uint8_t focused = 255;

    void init(uint8_t num, uint8_t display, std::string const& name) {
        emplace(num, Workspace{display, name, false, 0});
    }
    void focus_window(uint8_t num, uint64_t focused_window) { at(num).focused_window = focused_window; }
    void urgent(uint8_t num, bool urgent) { at(num).urgent = urgent; }
    void empty(uint8_t num) { erase(num); }

    Workspaces::const_iterator begin() const { return std::map<uint8_t, Workspace>::cbegin(); }
    Workspaces::const_iterator end() const { return std::map<uint8_t, Workspace>::cend(); }
};

struct Window final {
    // Workspace on which this window is located.
    uint8_t workspace;

    // Name of the window.
    std::string name;

    // Maybe store the urgent flag of a window?
};

// Map of uuid to window.
class Windows final : private std::map<uint64_t, Window> {
  public:
    uint8_t get_workspace_num(uint64_t num) { return at(num).workspace; }

    void open(uint64_t num, uint8_t workspace, std::string const& name) { emplace(num, Window{workspace, name}); }
    void title(uint64_t num, std::string const& name) { at(num).name = name; }
    void move(uint64_t num, uint8_t workspace) { at(num).workspace = workspace; }
    void close(uint64_t num) { erase(num); }

    bool contains(uint64_t id) const { return find(id) != end(); }
    // NOLINTNEXTLINE: Desired overload.
    Window const& operator[](uint64_t id) const { return at(id); }
};

#endif
