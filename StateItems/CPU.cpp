
#include<iostream>
#include<cstdlib>

#include"CPU.hpp"
#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string const CPU::temp_file_loc = "/sys/bus/acpi/devices/LNXTHERM:00/thermal_zone/temp";
string const CPU::load_file_loc = "/proc/loadavg";

CPU::CPU() :
  StateItem(5),
  sysmgr_cmd(mkTerminalCmd("htop"))
{
}

void CPU::performUpdate(void)
{
  char line[11];
  
  FILE* tfile = fopen(temp_file_loc.c_str(), "r");
  fgets(line, 10, tfile);
  fclose(tfile);
  cpu_temp = (int)strtol(line, nullptr, 0) / 1000;
  
  FILE* lfile = fopen(load_file_loc.c_str(), "r");
  fgets(line, 10, lfile);
  fclose(lfile);
  cpu_load = strtof(line, nullptr);
}

void CPU::print(void)
{
  startButton(sysmgr_cmd);
  
  dynamic_section(cpu_load, 0.7f, 3.0f);
  PRINT_ICON(icon_cpu);
  printf(" %.2f ", (double)cpu_load);
  
  dynamic_section((float)cpu_temp, 50.0f, 90.0f);
  cout << ' ' << cpu_temp << "°C ";
  separate(Left, white_on_black);
  
  stopButton();
}
