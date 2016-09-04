#ifndef __I3STATE_HEADER___
#define __I3STATE_HEADER___

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <memory>

#include "util.hpp"
#include "JSON/jsonParser.hpp"

class Workspace;

struct Output
{
  Output(uint8_t pos, std::string const& name);

  // Used to display the right display-tag in lemonbar
  uint8_t position;

  // Xorg name of the monitor: LVDS, eDP1, VGA1, HDMI2, DVI3
  std::string name;

  // The map always contains at least one element
  // maps from unique workspace num to the corresponging workspace
  // Per display Workspaces are sorted by their num
  std::map<uint8_t, std::shared_ptr<Workspace>> workspaces;

  // The current focused workspace
  std::shared_ptr<Workspace> focused_workspace;

  void update_from_tree(JSON const&);
};

class Workspace
{
private:
  void update_windows_from_tree(JSON const&);

public:
  Workspace(uint8_t num,
            std::string const& name,
            std::shared_ptr<Output> parent);
  virtual ~Workspace(void) = default;

  // unique number of workspace; nobody ever used more than 256 workspaces!
  uint8_t const num;
  // name of workspace as defined in ~/.i3/config
  std::string const name;
  // A workspae should always be bound to an output
  std::shared_ptr<Output> output;

  // urgent flag set by X
  bool urgent;

  // ID of the focused window. -1 if none
  long focused_window_id;

  void update_from_tree(JSON const&);
};

struct Window
{
  long id;
  std::string name;
  std::shared_ptr<Workspace> workspace;
};

class I3State : public Logger
{
private:
  int fd;
  bool& die;

public:
  bool valid;
  std::mutex mutex; // TODO private

  // UnMod/resize/XrandR-mode
  std::string mode;
  std::shared_ptr<Output> focused_output;

  std::map<std::string, std::shared_ptr<Output>> outputs;
  std::map<uint8_t, std::shared_ptr<Workspace>> workspaces;
  std::map<long, Window> windows;

public:
  explicit I3State(std::string& path, bool& die);
  ~I3State(void);

private:
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
};

#endif
