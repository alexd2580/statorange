
#include<iostream>
#include<cstdlib>

#include"CPU.hpp"
#include"../output.hpp"

using namespace std;

string CPU::temp_file_loc = "/sys/bus/acpi/devices/LNXTHERM:00/thermal_zone/temp";
string CPU::load_file_loc = "/proc/loadavg";

CPU::CPU() :
  StateItem(5)
{
}

CPU::~CPU()
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
  /*if(sys->cpu_load > 3.0f) { SEP_LEFT(warn_colors); }
  else if(sys->cpu_load > 1.0f) { SEP_LEFT(info_colors); }
  else { SEP_LEFT(neutral_colors); }*/
  dynamic_section(cpu_load, 0.7f, 3.0f);
  PRINT_ICON(icon_cpu);
  printf(" %.2f ", (double)cpu_load);
  
  /*if(sys->cpu_temp >= 70) { SEP_LEFT(warn_colors); }
  else if(sys->cpu_temp >= 55) { SEP_LEFT(info_colors); }
  else { SEP_LEFT(neutral_colors); }*/
  dynamic_section((float)cpu_temp, 50.0f, 90.0f);
  cout << ' ' << cpu_temp << "°C ";
  separate(Left, white_on_black);
}
