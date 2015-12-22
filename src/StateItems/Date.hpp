#ifndef __DATEXYZHEADER_LOL___
#define __DATEXYZHEADER_LOL___

#include<string>
#include<vector>
#include"../StateItem.hpp"

enum Identifier
{
  AbbrWeekDayName,//  a     locale's abbreviated weekday name (e.g., Sun)
  WeekDayName,    //  A     locale's full weekday name (e.g., Sunday)
  AbbrMonthName,  //  b     locale's abbreviated month name (e.g., Jan)
  MonthName,      //  B     locale's full month name (e.g., January)
  MonthDay,       //  d     day of month (e.g., 01)
  MonthDaySpc,    //  e     day of month, space padded
  Hour24,         //  H     hour (00..23)
  Hour12,         //  I     hour (01..12)
  YearDay,        //  j     day of year (001..366)
  Hour24Spc,      //  k     hour, space padded ( 0..23)
  Hour12Spc,      //  l     hour, space padded ( 1..12)
  Month,          //  m     month (01..12)
  Minute,         //  M     minute (00..59)
  AMPM,           //  p     AM or PM
  AMPMLow,        //  P     like p, but lower case
  Second,         //  S     second (00..60)
  WeekDay,        //  u     day of week (1..7); 1 is Monday
  Week,           //  W     week number of year, with Monday as first day of week (00..53)
  Year2,          //  y     last two digits of year (00..99)
  Year4           //  Y     year
};

enum TokenType
{
  DateItem, Symbol
};

typedef struct
{
  TokenType type;
  union
  {
    Identifier i;
    char c;
  };
} Token;

class Date : public StateItem
{
private:
  std::vector<Token> tokens;
  void parseFormat(std::string s);

  //DATE & TIME
  std::string time;

  bool update(void);
  void print(void);

public:
  static void settings(JSONObject& section);
  Date(JSONObject& item);
  virtual ~Date() {};
};

#endif
