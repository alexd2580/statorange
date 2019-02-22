#ifndef UTILS_I3_TREE_HPP
#define UTILS_I3_TREE_HPP

#include <algorithm>
#include <map>
#include <string>

#include "Lemonbar.hpp"

struct Output final {
    int x;
    int y;

    std::string const name;

    uint8_t workspace_focused = 255;
};

struct Workspace final {
    // Display on which this workspace is located.
    uint8_t display;

    // Name of workspace as defined in `~/.config/i3/config`.
    std::string const name;

    // Urgent flag set by X.
    bool urgent = false;

    uint64_t window_focused;
};

struct Window final {
    // Workspace on which this window is located.
    uint8_t workspace;

    // Name of the window.
    std::string name;

    // Maybe store the urgent flag of a window?
};

class I3Tree final {
  private:
    std::map<uint8_t, Output> outputs;

    // Map of unique number to workspace.
    std::map<uint8_t, Workspace> workspaces;

    // Map of uuid to window.
    std::map<uint64_t, Window> windows;

    decltype(outputs)::iterator get_output_by_name(std::string const& name) {
        auto is_searched_display = [&](auto const& pair) { return pair.second.name == name; };
        return std::find_if(outputs.begin(), outputs.end(), is_searched_display);
    }

    decltype(outputs)::const_iterator get_output_by_name(std::string const& name) const {
        auto is_searched_display = [&](auto const& pair) { return pair.second.name == name; };
        return std::find_if(outputs.cbegin(), outputs.cend(), is_searched_display);
    }

    // Get the display number for a display name. If no such display is present,
    // then return 255.
    uint8_t get_output_num_by_name(std::string const& name) const {
        auto display_iter = get_output_by_name(name);
        return display_iter == outputs.cend() ? 255 : display_iter->first;
    }

  public:
    // Focused workspace.
    uint8_t workspace_focused = 255;

    // Workspace events.
    void workspace_init(uint8_t num, std::string const& display_name, std::string const& name) {
        auto display_num = get_output_num_by_name(display_name);
        workspaces.emplace(num, Workspace{display_num, name, false, 0});
    }

    void workspace_focus(uint8_t num, std::string const& display_name) {
        workspace_focused = num;
        auto display_iter = get_output_by_name(display_name);
        if(display_iter == outputs.end()) {
            std::cerr << "Focussing workspace " << (int)num << " on display " << display_name
                      << " which does not exist." << std::endl;
            return;
        }

        auto& display = display_iter->second;
        display.workspace_focused = num;
    }

    void workspace_urgent(uint8_t num, bool urgent) { workspaces[num].urgent = urgent; }

    void workspace_move() { std::cout << "Not implemented!" << std::endl; }

    void workspace_empty(uint8_t num) {
        outputs[workspaces[num].display].workspace_focused = 255;
        workspaces.erase(num);
    }

    // Window events.
    void window_new() {
        // Already handled by `window_open`.
    }

    void window_title(uint64_t id, std::string const& name) { windows[id].name = name; }

    void window_focus(uint64_t window_id) {
        // When the focus changes to a partifular window, then the currently
        // focused workspace is the one the window is located on.
        windows[window_id].workspace = workspace_focused;
        workspaces[workspace_focused].window_focused = window_id;
    }

    void window_move(uint64_t window_id, std::string const& display_name) {
        // Unfocus the moved window from its workspace.
        auto const current_workspace = windows[window_id].workspace;
        workspaces[current_workspace].window_focused = 0;

        // Move the window to the new workspace.
        auto display_iter = get_output_by_name(display_name);
        if(display_iter == outputs.cend()) {
            std::cerr << "Moving window " << window_id << " to display " << display_name << " which does not exist."
                      << std::endl;
            return;
        }
        auto workspace_num = display_iter->second.workspace_focused;
        windows[window_id].workspace = workspace_num;
    }

    void window_close(uint64_t window_id) {
        auto const workspace_num = windows[window_id].workspace;
        windows.erase(window_id);
        workspaces[workspace_num].window_focused = 0;
    }

    // Const accessors.
    decltype(workspace_focused) get_workspace_focused() const { return workspace_focused; }

    decltype(workspaces) const& get_workspaces() const { return workspaces; }

    std::string const& get_window_name(uint64_t window_id) const { return windows.at(window_id).name; }

    // Utils.
    void display_add(int x, int y, std::string const& name) {
        uint8_t lower = 0;
        uint8_t upper = 255;
        auto compare_position = [&](int x1, int y1, int x2, int y2) { return x1 < x2 ? true : y1 < y2; };

        for(auto const& output_iter : outputs) {
            auto const& display = output_iter.second;
            if(compare_position(x, y, display.x, display.y)) {
                upper = std::min(upper, output_iter.first);
            } else {
                lower = std::max(lower, output_iter.first);
            }
        }

        outputs.emplace(upper / 2 + lower / 2, Output{x, y, name, 255});
    }

    uint8_t get_display_num_by_order(uint8_t order) {
        auto iter = outputs.cbegin();
        for(uint8_t i = 0; i < order; i++, iter++) {
        }
        return iter->first;
    }

    void window_open(uint64_t window_id, std::string const& display_name, std::string const& name) {
        auto display_iter = get_output_by_name(display_name);
        if(display_iter == outputs.cend()) {
            std::cerr << "Opening window " << window_id << ": " << name << " on display " << display_name
                      << " which does not exist." << std::endl;
            return;
        }
        auto workspace_num = display_iter->second.workspace_focused;
        windows.emplace(window_id, Window{workspace_num, name});
    }

    bool contains_window(uint64_t id) const { return windows.find(id) != windows.cend(); }

    bool contains_output(std::string const& name) const { return get_output_by_name(name) != outputs.cend(); }

    void reset() {
        outputs.clear();
        workspaces.clear();
        windows.clear();
        workspace_focused = 255;
    }

    // // NOLINTNEXTLINE: Desired overload.
    // Window const& operator[](uint64_t id) const { return at(id); }
    //
    // void clear() { std::map<uint8_t, Output>::clear(); }
};

#endif
