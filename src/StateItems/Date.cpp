
#include<iostream>
#include"Date.hpp"

#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string Date::get_date = "";

void Date::settings(JSONObject& section)
{
  get_date.assign(section["get_date"].string());
}

Date::Date(JSONObject& item) :
  StateItem(item)
{
}

bool Date::update(void)
{
  time = execute(get_date);
  time.pop_back(); //date prints newline
  return true;
}

void Date::print(void)
{
  separate(Left, active_colors);
  print_icon(icon_clock);
  cout << ' ' << time << ' ';
  separate(Left, white_on_black);
}
