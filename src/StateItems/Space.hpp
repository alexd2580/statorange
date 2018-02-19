#ifndef __PARTITIONSIZEXYZHEADER_LOL___
#define __PARTITIONSIZEXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <vector>

#include "../Lemonbar.hpp"
#include "../StateItem.hpp"
#include "../json_parser.hpp"

struct SpaceItem {
    std::string const mount_point;
    Lemonbar::Icon const icon;
    uint64_t size;
    uint64_t used;

    SpaceItem(std::string const& mpt, Lemonbar::Icon const icon_) : mount_point(mpt), icon(icon_) {
        size = 0;
        used = 0;
    }
};

std::ostream& operator<<(std::ostream&, SpaceItem const&);

class Space final : public StateItem {
  private:
    std::vector<SpaceItem> items;
    std::pair<bool, bool> get_space_usage(SpaceItem& dir);

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar&, uint8_t) override;

  public:
    explicit Space(JSON::Node const& item);
    virtual ~Space() override = default;
};

#endif
