
#include "Date.hpp"
#include <chrono> // std::chrono::system_clock
#include <ctime>
#include <iomanip> // std::put_time
#include <iostream>
#include <sstream>

#include "../output.hpp"
#include "../util.hpp"

using namespace std;

Date::Date(JSONObject& item) : StateItem(item)
{
  JSON* icon_id = item.has("icon");
  icon = icon_id == nullptr ? Icon::no_icon : parse_icon(icon_id->string());
  format = item["format"].string();
}

using std::chrono::system_clock;

bool Date::update(void)
{
  time_t tt = system_clock::to_time_t(system_clock::now());
  struct tm* ptm = localtime(&tt);
  ostringstream o;
  print_time(o, ptm, format.c_str());
  time = o.str();
  return true;
}

void Date::print(void)
{
  separate(Direction::left, Color::active);
  cout << icon << ' ' << time << ' ';
  separate(Direction::left, Color::white_on_black);
}
