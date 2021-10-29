#ifndef STATEITEMS_WEATHER_HPP
#define STATEITEMS_WEATHER_HPP

#include <string>
#include <chrono>

#include "Lemonbar.hpp"
#include "StateItem.hpp"
#include "json_parser.hpp"

class Weather final : public StateItem {
  private:
    static std::map<std::string, std::string> const icons;

    std::string temperature;
    std::string condition_code;
    std::string condition;

    std::chrono::minutes midnight_to_sunrise;
    std::chrono::minutes midnight_to_sunset;

    std::pair<bool, bool> update_raw() override;
    void print_raw(Lemonbar& bar, uint8_t) override; // NOLINT

  public:
    explicit Weather(JSON::Node const&);
};

#endif
