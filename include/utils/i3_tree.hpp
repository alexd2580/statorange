#ifndef UTILS_I3_TREE_HPP
#define UTILS_I3_TREE_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>

#include "Lemonbar.hpp"
#include "Logger.hpp"
#include "utils/multimap.hpp"

struct Output;
struct Workspace;
struct Window;

struct Output final {
    uint8_t rel_index;
    std::string const name;

    int x;
    int y;

    Workspace* visible_workspace;

    Output(uint8_t rel_index_, std::string const name_, int x_, int y_, Workspace* visible_workspace_)
        : rel_index(rel_index_), name(name_), x(x_), y(y_), visible_workspace(visible_workspace_) {}
};

class Outputs final : public Multimap<uint8_t, std::string, Output> {};

struct Workspace final {
    uint8_t const num;
    // Name of workspace as defined in `~/.config/i3/config`.
    std::string const name;
    // Urgent flag set by X.
    bool urgent;

    Output* output;
    Window* focused_window;
    Window* last_urgent_window;

    Workspace(uint8_t num_, std::string name_, bool urgent_, Output* output_, Window* focused_window_,
              Window* last_urgent_window_)
        : num(num_), name(name_), urgent(urgent_), output(output_), focused_window(focused_window_),
          last_urgent_window(last_urgent_window_) {}
};

class Workspaces final : public std::map<uint8_t, Workspace> {
  public:
    Workspace* focused;

    // Const accessors.
    bool is_focused(Workspace const& workspace) const { return &workspace == focused; }
};

struct Window final {
    uint64_t const id;
    std::string name;
    bool urgent;

    Workspace* workspace;

    Window(uint64_t id_, std::string name_, bool urgent_, Workspace* workspace_)
        : id(id_), name(name_), urgent(urgent_), workspace(workspace_) {}
};

class Windows : public std::map<uint64_t, Window> {
  public:
    bool contains(uint64_t id) const { return find(id) != cend(); }

    std::string const& name_of(uint64_t id) const { return at(id).name; }
};

class I3Tree final {
  private:
    Outputs m_outputs;
    Workspaces m_workspaces;
    Windows m_windows;

  public:
    Outputs const& outputs = m_outputs;
    Workspaces const& workspaces = m_workspaces;
    Windows const& windows = m_windows;

    // Output utils.
    Output& display_add(int x, int y, std::string const& name) {
        uint8_t lower = 0;
        uint8_t upper = 255;
        auto first_before_second = [&](int x1, int y1, int x2, int y2) { return x1 < x2 || (x1 == x2 && y1 < y2); };

        for(auto const& output : m_outputs.iterableFromIndex1()) {
            if(first_before_second(x, y, output.x, output.y)) {
                upper = std::min(upper, output.rel_index);
            } else {
                lower = std::max(lower, output.rel_index);
            }
        }
        uint8_t index = (upper + lower) / 2;
        return *m_outputs.emplace(index, name, index, name, x, y, m_workspaces.focused).first;
    }

    // Workspace events.
    Workspace& workspace_init(uint8_t num, std::string const& display_name, std::string const& name) {
        return m_workspaces.try_emplace(num, num, name, false, &m_outputs.at(display_name), nullptr, nullptr)
            .first->second;
    }

    void workspace_focus(uint8_t num, std::string const& display_name) {
        auto& workspace = m_workspaces.at(num);
        m_workspaces.focused = &workspace;
        m_outputs.at(display_name).visible_workspace = &workspace;
    }

    void workspace_urgent(uint8_t num, bool urgent) { m_workspaces.at(num).urgent = urgent; }

    void workspace_move() { LOG << "`workspace_move` not implemented!" << std::endl; }

    void workspace_empty(uint8_t num) {
        auto& output = *m_workspaces.at(num).output;
        if(output.visible_workspace->num == num) {
            LOG << "Unfocusing a workspace on a workspace which still has focus on it." << std::endl;
        }
        m_workspaces.erase(num);
    }

    // Window events.
    void window_new() {
        // Already handled by `window_open`.
    }

    // TODO What's the difference to `window_new`?
    void window_open(uint64_t window_id, std::string const& display_name, std::string const& name) {
        auto& output = m_outputs.at(display_name);
        auto& workspace = output.visible_workspace;
        m_windows.emplace(window_id, Window{window_id, name, false, workspace});
    }

    void window_title(uint64_t id, std::string const& name) { m_windows.at(id).name = name; }

    void window_focus(uint64_t window_id) {
        // Do i need this?
        // When the focus changes to a particular window, then the currently
        // focused workspace is the one the window is located on.
        // windows[window_id].workspace = workspaces.focused;
        auto& window = m_windows.at(window_id);
        window.workspace->focused_window = &window;
    }

    void window_urgent(uint64_t window_id, bool urgent) {
        auto& window = m_windows.at(window_id);
        window.urgent = urgent;

        auto& workspace = *window.workspace;
        if(urgent) {
            workspace.last_urgent_window = &window;
        } else if(workspace.last_urgent_window == &window) {
            workspace.last_urgent_window = nullptr;
        }
    }

    void window_move(uint64_t window_id, std::string const& display_name) {
        // Unfocus the moved window from its workspace.
        auto& window = m_windows.at(window_id);
        auto& old_workspace = *window.workspace;
        old_workspace.focused_window = nullptr;

        auto& new_output = m_outputs.at(display_name);
        auto& new_workspace = new_output.visible_workspace;
        window.workspace = new_workspace;
    }

    void window_close(uint64_t window_id) {
        auto& window = m_windows.at(window_id);
        auto& workspace = *window.workspace;
        workspace.focused_window = nullptr;
        if(workspace.last_urgent_window == &window) {
            workspace.last_urgent_window = nullptr;
        }
        m_windows.erase(window_id);
    }

    void reset() {
        m_outputs.clear();
        m_workspaces.clear();
        m_windows.clear();
        m_workspaces.focused = nullptr;
    }

    // // NOLINTNEXTLINE: Desired overload.
    // Window const& operator[](uint64_t id) const { return at(id); }
    //
    // void clear() { std::map<uint8_t, Output>::clear(); }
};

#endif
