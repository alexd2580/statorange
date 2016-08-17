#ifndef __BATTERYXYZHEADER_LOL___
#define __BATTERYXYZHEADER_LOL___

#include <string>
#include "../StateItem.hpp"
#include "../JSON/jsonParser.hpp"

enum class BatStatus
{
  not_found,
  charging,
  full,
  discharging
};

class Battery : public StateItem
{
private:
  std::string bat_file_loc;

  bool cached;
  std::string printString;

  // BAT
  BatStatus status;
  long dischargeRate;
  long maxCapacity;
  long currentLevel;

  bool update(void);
  void print(void);

public:
  Battery(JSON const&);
  virtual ~Battery(void) = default;
};

#endif
