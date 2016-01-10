#ifndef __STATEITEMHEADER_LOL___
#define __STATEITEMHEADER_LOL___

#include <ctime>
#include <vector>
#include <string>

#include "JSON/jsonParser.hpp"

class StateItem
{
private:
  static std::vector<StateItem*> states;

  std::string module_name;
  time_t cooldown;
  time_t last_updated;
  bool button;
  std::string button_command;
  bool valid;

  void wrap_update(void);
  void force_update(void);
  void wrap_print(void);

protected:
  virtual bool update(void) = 0;
  virtual void print(void) = 0;
  StateItem(JSONObject& item);

public:
  virtual ~StateItem(){};

  static void init(JSONObject& config);
  static void updates(void);
  static void forceUpdates(void);
  static void printState(void);
  static void close(void);
};

#endif
