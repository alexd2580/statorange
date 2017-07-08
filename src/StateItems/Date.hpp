#ifndef __DATEXYZHEADER_LOL___
#define __DATEXYZHEADER_LOL___

#include <ostream>
#include <string>

#include "../StateItem.hpp"
#include "../output.hpp"

class Date : public StateItem
{
  private:
    std::string const format;
    std::string time;

    bool update(void);
    void print(std::ostream&, uint8_t);

  public:
    Date(JSON::Node const& item);
    virtual ~Date(void) = default;
};

#endif
