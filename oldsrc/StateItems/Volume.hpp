#ifndef __VOLUMEXYZHEADER__
#define __VOLUMEXYZHEADER__

#include <string>
#include <ostream>

#include "StateItem.hpp"
#include "output.hpp"

class Volume : public StateItem
{
  private:
    std::string const card;
    std::string const mixer;

    bool mute;
    unsigned short volume;

    bool update(void);
    void print(std::ostream&, uint8_t);

  public:
    Volume(JSON::Node const& item);
    virtual ~Volume(void) = default;
};

#endif
