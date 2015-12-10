#ifndef __BATTERYXYZHEADER_LOL___
#define __BATTERYXYZHEADER_LOL___

#include<string>
#include"../StateItem.hpp"

enum BatStatus
{
  NotFound, Charging, Full, Discharging
};

class Battery : public StateItem
{
private:
  bool cached;
  std::string printString;

  //BAT
  BatStatus status;
  long dischargeRate;
  long maxCapacity;
  long currentLevel;
  
  static std::string bat_file_loc;
  bool update(void);
  void print(void);
public:
  Battery();
  virtual ~Battery() {};
};

#endif
