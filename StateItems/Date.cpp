
#include<iostream>
#include"Date.hpp"

#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string Date::command = "date +%Y-%m-%d\\ %H:%M";

Date::Date() : 
  StateItem(30)
{
}

Date::~Date()
{
}

void Date::performUpdate(void)
{
  time = execute(command);
  time.pop_back(); //date prints newline
}

void Date::print(void)
{
  separate(Left, active_colors);
  print_icon(icon_clock);
  cout << ' ' << time << ' ';
  separate(Left, white_on_black);
}
