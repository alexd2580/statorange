
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cstring>

#include "CPU.hpp"
#include "../output.hpp"
#include "../util.hpp"

using namespace std;

string CPU::temp_file_loc = "/dev/zero";
string CPU::load_file_loc = "/dev/zero";

void CPU::settings(JSONObject& section)
{
  temp_file_loc.assign(section["temperature_file"].string());
  load_file_loc.assign(section["load_file"].string());
}

CPU::CPU(JSONObject& item) : StateItem(item), Logger("[CPU]", cerr)
{
  print_string = "";
  cached = false;
  cpu_temp = 0;
  cpu_load = 0.0f;
}

bool CPU::update(void)
{
  cached = false;
  char line[11];
  char* res;

  FILE* tfile = fopen(temp_file_loc.c_str(), "r");
  if(tfile == nullptr)
  {
    log() << "Couldn't read temperature file [" << temp_file_loc << "]:" << endl << strerror(errno) << endl;
    return false;
  }
  res = fgets(line, 10, tfile);
  fclose(tfile);
  if(res != line)
  {
    log() << "Couldn't read temperature file [" << temp_file_loc << "]:" << endl << strerror(errno) << endl;
    return false;
  }
  cpu_temp = (int)strtol(line, nullptr, 0) / 1000;

  FILE* lfile = fopen(load_file_loc.c_str(), "r");
  if(lfile == nullptr)
  {
    log() << "Couldn't read load file [" << load_file_loc << "]:" << endl << strerror(errno) << endl;
    return false;
  }
  res = fgets(line, 10, lfile);
  fclose(lfile);
  if(res != line)
  {
    log() << "Couldn't read load file [" << load_file_loc << "]:" << endl << strerror(errno) << endl;
    return false;
  }
  cpu_load = strtof(line, nullptr);

  return true;
}

void CPU::print(void)
{
  if(!cached)
  {
    ostringstream o;
    dynamic_section(cpu_load, 0.7f, 3.0f, o);
    print_icon(icon_cpu, o);
    o << ' ' << cpu_load << ' ';

    dynamic_section((float)cpu_temp, 50.0f, 90.0f, o);
    o << ' ' << cpu_temp << "°C ";
    separate(Left, white_on_black, o);

    print_string = o.str();
    cached = true;
  }

  cout << print_string;
}
