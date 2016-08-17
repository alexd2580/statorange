#ifndef __DATEXYZHEADER_LOL___
#define __DATEXYZHEADER_LOL___

#include <string>

#include "../StateItem.hpp"
#include "../output.hpp"

class Date : public StateItem
{
private:
  Icon icon;
  std::string format;

  // DATE & TIME
  std::string time;

  bool update(void);
  void print(void);

public:
  Date(JSON const& item);
  virtual ~Date(void) = default;
};

#endif
