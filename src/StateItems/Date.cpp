
#include<iostream>
#include<ctime>
#include<iomanip>      // std::put_time
#include<chrono>       // std::chrono::system_clock
#include<sstream>
#include"Date.hpp"

#include"../output.hpp"
#include"../util.hpp"

using namespace std;

/*void Date::settings(JSONObject&)
{
}*/

Date::Date(JSONObject& item) :
  StateItem(item)
{
  format = item["format"].string();
}

using std::chrono::system_clock;

bool Date::update(void)
{
  time_t tt = system_clock::to_time_t (system_clock::now());
  struct tm* ptm = localtime(&tt);
  ostringstream o;
  o << put_time(ptm, format.c_str());
  time = o.str();
  return true;
}

void Date::print(void)
{
  separate(Left, active_colors);
  print_icon(icon_clock);
  cout << ' ' << time << ' ';
  separate(Left, white_on_black);
}
