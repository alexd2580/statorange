
#include<iostream>
#include"Date.hpp"

#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string Date::command = "date +%Y-%m-%d\\ %H:%M";

Date::Date() : 
  StateItem("Date", 30)
{
}

bool Date::update(void)
{
  time = execute(command);
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
