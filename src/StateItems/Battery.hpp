#ifndef __BATTERYXYZHEADER_LOL___
#define __BATTERYXYZHEADER_LOL___

#include <ostream>
#include <string>

#include "../json_parser.hpp"
#include "../StateItem.hpp"

enum class BatStatus
{
    not_found,
    charging,
    full,
    discharging
};

class Battery final : public StateItem
{
  private:
    std::string const bat_file_loc;

    BatStatus status;
    long discharge_rate;
    long max_capacity;
    long current_level;

    bool update(void) override;
    void print(std::ostream&, uint8_t) override;

  public:
    Battery(JSON::Node const&);
    ~Battery(void) = default;
};

#endif
