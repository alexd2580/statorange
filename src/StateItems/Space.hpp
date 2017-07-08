#ifndef __PARTITIONSIZEXYZHEADER_LOL___
#define __PARTITIONSIZEXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <vector>

#include "../StateItem.hpp"
#include "../json_parser.hpp"
#include "../output.hpp"

struct SpaceItem
{
    std::string const mount_point;
    BarWriter::Icon const icon;
    float size;
    float used;
    std::string unit;

    SpaceItem(
        std::string const& mpt,
        BarWriter::Icon const icon_,
        float size_,
        float used_,
        std::string const& unit_)
        : mount_point(mpt), icon(icon_)
    {
        size = size_;
        used = used_;
        unit = unit_;
    }
};

class Space final : public StateItem
{
  private:
    std::vector<SpaceItem> items;
    bool get_space_usage(SpaceItem& dir);

    bool update(void) override;
    void print(std::ostream&, uint8_t) override;

  public:
    explicit Space(JSON::Node const& item);
};

#endif
