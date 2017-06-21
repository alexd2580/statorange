#ifndef __I3WKSPCXYZHEADER_LOL___
#define __I3WKSPCXYZHEADER_LOL___

#include <ostream>
#include <string>

#include "../JSON/json_parser.hpp"
#include "../StateItem.hpp"
#include "../util.hpp"

class I3Workspaces : public StateItem
{
  private:
    I3State i3;

    bool update(void);
    void print(std::ostream&, uint8_t);

  public:
    I3Workspaces(JSON const& item);
    virtual ~I3Workspaces(void) = default;
};

#endif
