#ifndef __DATEXYZHEADER_LOL___
#define __DATEXYZHEADER_LOL___

#include <ostream>
#include <string>
#include <utility>

#include "../Lemonbar.hpp"
#include "../StateItem.hpp"
#include "../json_parser.hpp"

class Date final : public StateItem {
  private:
    std::string const format;
    std::string time;

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar&, uint8_t) override;

  public:
    explicit Date(JSON::Node const& item);
    virtual ~Date() override = default;
};

#endif
