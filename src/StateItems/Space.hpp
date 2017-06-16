#ifndef __PARTITIONSIZEXYZHEADER_LOL___
#define __PARTITIONSIZEXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <vector>

#include "../JSON/json_parser.hpp"
#include "../StateItem.hpp"
#include "../output.hpp"

struct SpaceItem
{
    std::string mount_point;
    BarWriter::Icon icon;
    float size;
    float used;
    std::string unit;
};

class Space : public StateItem
{
  private:
    std::vector<SpaceItem> items;
    bool get_space_usage(SpaceItem& dir);

    bool update(void);
    void print(std::ostream&, uint8_t);

  public:
    explicit Space(JSON const& item);
    virtual ~Space(void) = default;
};

#endif
