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
  // Used to display the right display-tag in lemonbar
  uint8_t position;

  // Xorg name of the monitor: LVDS, eDP1, VGA1, HDMI2, DVI3
  std::string name;

  // The map always contains at least one element
  // maps from unique workspace num to the corresponging workspace
  // Per display Workspaces are sorted by their num
  std::map<uint8_t, std::unique_ptr<Workspace>> workspaces;
};

class Workspace
{
private:
  Workspace(JSON const& from_workspace_list, Output& parent);

public:
  /**
   * unique number of workspace
   * nobody ever used more than 256 workspaces!
   */
  uint8_t const num;
  std::string const name;
  // A workspae is always bound to an output
  Output& output;

  bool visible;
  bool focused;
  bool urgent;

  long focused_window_id;          // -1 for none
  std::string focused_window_name; // undefined when none

  // Parses an entry from GET_WORKSPACES
  // requires the map of outputs to match workspace and output
  static void register_workspace(JSON const& json,
                                 std::map<std::string, Output>& outputs);
  virtual ~Workspace(void) = default;
};

class I3State : public Logger
{
private:
  int fd;
  bool& die;

public:
  std::mutex mutex; // TODO private

  explicit I3State(std::string& path, bool& die);
  ~I3State(void);
  bool valid;

  std::string mode;
  std::map<std::string, Output> outputs;
  std::map<long, std::string> window_titles;

  void init_layout(void);
  void get_outputs(void);
  void get_workspaces(void);

  void workspace_init(uint8_t num);
  void workspace_status(void);
  void workspace_empty(uint8_t num);
};

#endif
