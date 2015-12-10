#ifndef __DATEXYZHEADER_LOL___
#define __DATEXYZHEADER_LOL___

#include<string>
#include"../StateItem.hpp"

class Date : public StateItem
{
private:
  //DATE & TIME
  std::string time;
  static std::string command;

  bool update(void);
  void print(void);
public:
  Date();
  virtual ~Date() {};
};

#endif
