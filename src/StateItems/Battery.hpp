#ifndef __BATTERYXYZHEADER_LOL___
#define __BATTERYXYZHEADER_LOL___

#include <ostream>
#include <string>

#include "../Lemonbar.hpp"
#include "../StateItem.hpp"
#include "../json_parser.hpp"

class Battery final : public StateItem {
  private:
    enum class Status { not_found, charging, full, discharging };

    std::string const bat_file_loc;

    Status status;
    int64_t discharge_rate;
    int64_t max_capacity;
    int64_t current_level;

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar&, uint8_t) override;

  public:
    explicit Battery(JSON::Node const&);
};

#endif
