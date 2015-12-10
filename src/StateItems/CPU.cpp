
#include<iostream>
#include<cstdlib>
#include<sstream>

#include"CPU.hpp"
#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string const CPU::temp_file_loc = "/sys/bus/acpi/devices/LNXTHERM:00/thermal_zone/temp";
string const CPU::load_file_loc = "/proc/loadavg";

CPU::CPU() :
  StateItem("CPU", 5),
  sysmgr_cmd(mkTerminalCmd("htop"))
{
    cached = false;
    cpu_temp = 0;
    cpu_load = 0.0f;
}

bool CPU::update(void)
{
  cached = false;
  char line[11];
  
  FILE* tfile = fopen(temp_file_loc.c_str(), "r");
  FAIL_ON_FALSE(tfile != nullptr)
  fgets(line, 10, tfile);
  fclose(tfile);
  cpu_temp = (int)strtol(line, nullptr, 0) / 1000;
  
  FILE* lfile = fopen(load_file_loc.c_str(), "r");
  FAIL_ON_FALSE(lfile != nullptr)
  fgets(line, 10, lfile);
  fclose(lfile);
  cpu_load = strtof(line, nullptr);
  
  return true;
}

void CPU::print(void)
{
  if(!cached)
  {
    ostringstream o;
    startButton(sysmgr_cmd, o);
    
    dynamic_section(cpu_load, 0.7f, 3.0f, o);
    print_icon(icon_cpu, o);
    o << ' ' << cpu_load << ' ';
    
    dynamic_section((float)cpu_temp, 50.0f, 90.0f, o);
    o << ' ' << cpu_temp << "°C ";
    separate(Left, white_on_black, o);
    
    stopButton(o);
    
    printString = o.str();
    cached = true;
  }
  
  cout << printString;
}
