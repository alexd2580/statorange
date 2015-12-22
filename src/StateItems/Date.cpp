
#include<iostream>
#include"Date.hpp"

#include"../output.hpp"
#include"../util.hpp"

using namespace std;

void Date::settings(JSONObject& section)
{
  //get_date.assign(section["get_date"].string());
}

Date::Date(JSONObject& item) :
  StateItem(item)
{
}

bool Date::update(void)
{
  //time = execute(get_date); TODO
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

enum WeekDays
{
  Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday
};
enum Months
{
  January, February, March, April, May, June, July, August, September, October, November, December
};

namespace Abbr
{
  enum WeekDays
  {
    Mon, Tue, Wed, Thu, Fri, Sat, Sun
  };
  enum Months
  {
    Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
  };
}

void Date::parseFormat(string s)
{
  tokens.clear();
  Token token;
  for(auto i = s.begin(); i != s.end(); i++)
  {
    token.type = DateItem;
    switch(*i)
    {
      #define TOKEN(l, a) case l: token.i = a; break;
      TOKEN('a', AbbrWeekDayName)
      TOKEN('A', WeekDayName)
      TOKEN('b', AbbrMonthName)
      TOKEN('B', MonthName)
      TOKEN('d', MonthDay)
      TOKEN('e', MonthDaySpc)
      TOKEN('H', Hour24)
      TOKEN('I', Hour12)
      TOKEN('j', YearDay)
      TOKEN('k', Hour24Spc)
      TOKEN('l', Hour12Spc)
      TOKEN('m', Month)
      TOKEN('M', Minute)
      TOKEN('p', AMPM)
      TOKEN('P', AMPMLow)
      TOKEN('S', Second)
      TOKEN('u', WeekDay)
      TOKEN('W', Week)
      TOKEN('y', Year2)
      TOKEN('Y', Year4)
      default:
        token.type = Symbol;
        token.c = *i;
        break;
    }
    tokens.push_back(token);
  }
}
