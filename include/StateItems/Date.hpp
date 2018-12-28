#ifndef STATEITEMS_DATE_HPP
#define STATEITEMS_DATE_HPP

#include <ostream>
#include <string>
#include <utility>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"

class Date final : public StateItem {
  private:
    std::string const format;
    std::string time;

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar&, uint8_t) override;

  public:
    explicit Date(JSON::Node const& item);
};

#endif
