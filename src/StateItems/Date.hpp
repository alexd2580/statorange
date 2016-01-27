#ifndef __DATEXYZHEADER_LOL___
#define __DATEXYZHEADER_LOL___

#include <string>
#include "../StateItem.hpp"

class Date : public StateItem
{
private:
  std::string format;

  // DATE & TIME
  std::string time;

  bool update(void);
  void print(void);

public:
  // static void settings(JSONObject& section);
  Date(JSONObject& item);
  virtual ~Date(){}
};

#endif
