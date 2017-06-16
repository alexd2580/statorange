#ifndef __BATTERYXYZHEADER_LOL___
#define __BATTERYXYZHEADER_LOL___

#include <ostream>
#include <string>

#include "../JSON/json_parser.hpp"
#include "../StateItem.hpp"

enum class BatStatus
{
    not_found,
    charging,
    full,
    discharging
};

class Battery : public StateItem
{
  private:
    std::string bat_file_loc;

    bool cached;
    std::string print_string;

    // BAT
    BatStatus status;
    long discharge_rate;
    long max_capacity;
    long current_level;

    bool update(void);
    void print(std::ostream&, uint8_t);

  public:
    Battery(JSON const&);
    virtual ~Battery(void) = default;
};

#endif
