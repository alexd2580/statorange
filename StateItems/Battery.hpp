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
  //BAT
  BatStatus status;
  long dischargeRate;
  long maxCapacity;
  long currentLevel;
  
  static std::string bat_file_loc;
  void performUpdate(void);
  void print(void);
public:
  Battery();
  virtual ~Battery();
};

#endif
