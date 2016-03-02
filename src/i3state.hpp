#ifndef __I3STATE_HEADER___
#define __I3STATE_HEADER___

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <mutex>

#include "util.hpp"

struct Workspace
{
  /**
   * unique number of workspace
   * nobody ever used more than 256 workspaces!
   */
  uint8_t num;
  std::string name;
  bool visible;
  bool focused;
  bool urgent;
  int output;

  long focusedAppID;      // -1 indicates unknown
  std::string focusedApp; // undefined when unknown
};

class I3State : public Logger
{
private:
  int fd;
  bool& die;

public:
  std::mutex mutex; // TODO private

  explicit I3State(std::string& path, bool& die);
  ~I3State();
  bool valid;

  std::string mode;

  std::map<std::string, uint8_t> outputs;
  std::vector<Workspace> workspaces;

  size_t focusedWorkspace;

  void parseOutputs(void);
  void parseWorkspaces(void);

  void updateOutputs(void);
  void workspaceInit(uint8_t num);
  void updateWorkspaceStatus(void);
  void workspaceEmpty(uint8_t num);
};

#endif
