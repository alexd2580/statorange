#ifndef __DATEXYZHEADER_LOL___
#define __DATEXYZHEADER_LOL___

#include<string>
#include"../StateItem.hpp"

class Date : public StateItem
{
private:
  //DATE & TIME
  std::string time;
  static std::string get_date;

  bool update(void);
  void print(void);
public:
  static void settings(JSONObject& section);
  Date(JSONObject& item);
  virtual ~Date() {};
};

#endif
