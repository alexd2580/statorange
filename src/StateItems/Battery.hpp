#ifndef __BATTERYXYZHEADER_LOL___
#define __BATTERYXYZHEADER_LOL___

#include<string>
#include"../StateItem.hpp"
#include"../JSON/jsonParser.hpp"

enum BatStatus
{
  NotFound, Charging, Full, Discharging
};

class Battery : public StateItem
{
private:
  static std::string bat_file_loc;

  bool cached;
  std::string printString;

  //BAT
  BatStatus status;
  long dischargeRate;
  long maxCapacity;
  long currentLevel;

  bool update(void);
  void print(void);
public:
  static void settings(JSONObject&);
  Battery(JSONObject&);
  virtual ~Battery() {};
};

#endif
