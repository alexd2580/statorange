#ifndef __STATEITEMHEADER_LOL___
#define __STATEITEMHEADER_LOL___

#include<ctime>
#include<vector>


class StateItem
{
private:
  static std::vector<StateItem*> states;

  time_t cooldown;
  time_t last_updated;
  void update(void);
  void forceUpdate(void);
protected:
  virtual void performUpdate(void) = 0;
  virtual void print(void) = 0;
public:
  StateItem(time_t cooldown);
  virtual ~StateItem();
  
  static void init(void);
  static void updates(void);
  static void forceUpdates(void);
  static void printState(void);
  static void close(void);
};

#endif
