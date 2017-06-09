#ifndef __I3STATE_HEADER___
#define __I3STATE_HEADER___

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../JSON/json_parser.hpp"
#include "../Logger.hpp"
#include "../util.hpp"
#include "../StateItem.hpp"

struct Workspace;

struct Output
{
    // Used to display the right display-tag in lemonbar
    uint8_t const position;

    // Xorg name of the monitor: LVDS, eDP1, VGA1, HDMI2, DVI3
    std::string const name;

    // The map always contains at least one element
    // maps from unique workspace num to the corresponging workspace
    // Per display Workspaces are sorted by their num
    std::map<uint8_t, std::shared_ptr<Workspace>> workspaces;

    // The current focused workspace
    std::shared_ptr<Workspace> focused_workspace;

    Output(uint8_t pos, std::string const& name_) : position(pos), name(name_)
    {
    }
};

struct Workspace
{
    // Unique number of workspace; nobody ever used more than 256 workspaces!
    uint8_t const num;

    // Name of workspace as defined in ~/.config/i3/config.
    std::string const name;

    // A workspae should always be bound to an output.
    std::shared_ptr<Output> output;

    // Urgent flag set by X.
    bool urgent;

    // ID of the focused window. -1 if none.
    long focused_window_id;

    Workspace(uint8_t num_,
              std::string const& name_,
              std::shared_ptr<Output> parent)
        : num(num_), name(name_), output(parent)
    {
        urgent = false;
        focused_window_id = -1;
    }
};

struct Window
{
    long id;
    std::string name;
    std::shared_ptr<Workspace> workspace;
};

class I3State  : public StateItem
{
  private:
    int push_socket;

  public:
    bool valid;

    // i3-modes, defined in the i3-config.
    std::string mode;
    std::shared_ptr<Output> focused_output;

    std::map<std::string, std::shared_ptr<Output>> outputs;
    std::map<uint8_t, std::shared_ptr<Workspace>> workspaces;
    std::map<long, Window> windows;

  public:
    explicit I3State(std::string const& path);
    virtual ~I3State();

    void handle_event(int fd);

  private:
    // Send a message to i3, and receive a response.
    std::unique_ptr<char[]> message(uint32_t type);
    void register_workspace(uint8_t num,
                            std::string const& name,
                            std::string const& output_name,
                            bool focused);

    void get_outputs(void);
    void get_workspaces(void);

  public:
    void init_layout(void);

    void workspace_init(JSON const& current);
    // also determine output monitor via get_outputs?
    void workspace_focus(JSON const& current); // does not contain visible
    void workspace_urgent(JSON const& current);
    void workspace_empty(JSON const& current);

    /**
     * Updates the current output/workspace/window mapping,
     * setting the boolean flags and the currently focused window id.
     * Needs to be called after a window focus event.
     */
    void update_from_tree(void);
    void update_outputs_from_tree(JSON const&);
    void update_workspaces_from_tree(std::shared_ptr<Output>, JSON const&);
    void update_windows_from_tree(std::shared_ptr<Workspace>, JSON const&);
};

#endif
