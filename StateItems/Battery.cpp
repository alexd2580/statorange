
#include<iostream>
#include<cstdlib>
#include<cstring>
#include<sstream>

#include"Battery.hpp"
#include"../output.hpp"

using namespace std;

string Battery::bat_file_loc = "/sys/class/power_supply/BAT0/uevent";

Battery::Battery() :
  StateItem(15), cached(false), printString("")
{
}

void Battery::performUpdate(void)
{
  cached = false;
  char line[201];
  //battery
  FILE* bfile = fopen(bat_file_loc.c_str(), "r");
  if(bfile == nullptr)
    status = NotFound; //Probably no battery
  else
  {
    while(fgets(line, 200, bfile) != nullptr)
    {
      if(strncmp(line+13, "STATUS", 6) == 0)
      {
        if(strncmp(line+20, "Full", 4) == 0)
          status = Full;
        else if(strncmp(line+20, "Charging", 8) == 0)
          status = Charging;
        else if(strncmp(line+20, "Discharging", 11) == 0)
          status = Discharging;
        else
          status = Full;
      }
      else if(strncmp(line+13, "POWER_NOW", 9) == 0)
        dischargeRate = strtol(line+23, NULL, 0);
      else if(strncmp(line+13, "ENERGY_FULL_DESIGN", 18) == 0)
        maxCapacity = strtol(line+32, NULL, 0);
      else if(strncmp(line+13, "ENERGY_NOW", 10) == 0)
        currentLevel = strtol(line+24, NULL, 0);
    }
    fclose(bfile);
  }
}

void Battery::print(void)
{
  if(!cached)
  {
    ostringstream o;
    if(status != NotFound)
    {
      switch(status)
      {
      case Charging:
        separate(Left, neutral_colors, o);
        o << " BAT charging " << 100*currentLevel/maxCapacity << "%% ";
        break;
      case Full:
        separate(Left, info_colors, o);
        o << " BAT full ";
        break;
      case Discharging:
      {
        long rem_minutes = 60*currentLevel / dischargeRate;
        /*if(rem_minutes < 20) {  SEP_LEFT(warn_colors); }
        else if(rem_minutes < 60) { SEP_LEFT(info_colors); }
        else { SEP_LEFT(neutral_colors); }*/
        dynamic_section((float)-rem_minutes, -60.0f, -10.0f, o);
        o << " BAT " << (int)(100*currentLevel / maxCapacity) << "%% ";
        separate(Left, nullptr, o);

        long rem_hrOnly = rem_minutes / 60;
        o << (rem_hrOnly < 10 ? " 0" : " ") << rem_hrOnly;

        long rem_minOnly = rem_minutes % 60;
        o << (rem_minOnly < 10 ? ":0" : ":") << rem_minOnly << ' ';
        
        break;
      }
      case NotFound:
      default:
        break;
      }
      separate(Left, white_on_black, o);
    
    }
    printString = o.str();
    cached = true;
  }
  cout << printString;
}
