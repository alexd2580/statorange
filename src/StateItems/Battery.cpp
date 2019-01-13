#include <utility>

#include <cstdlib>
#include <cstring>

#include "StateItems/Battery.hpp"

#include "Lemonbar.hpp"

// TODO Use utility functions.
std::pair<bool, bool> Battery::update_raw() {
    std::unique_ptr<FILE, int (*)(FILE*)> file(fopen(bat_file_loc.c_str(), "re"), fclose);
    if(file == nullptr) {
        status = Status::not_found;
    } else {
        char line[201];
        auto line_ptr = static_cast<char*>(line);
        while(fgets(line_ptr, 200, file.get()) != nullptr) {
            if(strncmp(line_ptr + 13, "STATUS", 6) == 0) {
                if(strncmp(line_ptr + 20, "Full", 4) == 0) {
                    status = Status::full;
                } else if(strncmp(line_ptr + 20, "Charging", 8) == 0) {
                    status = Status::charging;
                } else if(strncmp(line_ptr + 20, "Discharging", 11) == 0) {
                    status = Status::discharging;
                } else {
                    status = Status::full;
                }
            } else if(strncmp(line_ptr + 13, "POWER_NOW", 9) == 0) {
                discharge_rate = strtol(line_ptr + 23, nullptr, 0);
            } else if(strncmp(line_ptr + 13, "ENERGY_FULL_DESIGN", 18) == 0) {
                max_capacity = strtol(line_ptr + 32, nullptr, 0);
            } else if(strncmp(line_ptr + 13, "ENERGY_NOW", 10) == 0) {
                current_level = strtol(line_ptr + 24, nullptr, 0);
            }
        }
    }

    return {true, true};
}

void Battery::print_raw(Lemonbar& bar, uint8_t display) {
    (void)display;
    if(status == Status::not_found) {
        return;
    }

    switch(status) {
    case Status::charging: {
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::neutral);
        auto const relative = 100 * current_level / max_capacity;
        std::map<uint8_t, std::string> const charging_icons = {
            {10, "\uf585"}, {20, "\uf586"}, {30, "\uf587"},  {50, "\uf588"},
            {70, "\uf589"}, {80, "\uf58a"}, {255, "\uf584"},
        };
        auto is_searched_value = [&](auto const& pair) { return pair.first > relative; };
        auto const icon = std::find_if(charging_icons.cbegin(), charging_icons.cend(), is_searched_value)->second;

        bar() << icon << " " << relative << "% ";
        break;
    }
    case Status::full:
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::info);
        bar() << " \uf584 ";
        break;
    case Status::discharging: {
        int64_t rem_minutes = 60 * current_level / discharge_rate;
        /*if(rem_minutes < 20) {  SEP_LEFT(warn_colors); }
        else if(rem_minutes < 60) { SEP_LEFT(info_colors); }
        else { SEP_LEFT(neutral_colors); }*/
        auto colors = Lemonbar::section_colors(-static_cast<float>(rem_minutes), -60.0f, -10.0f);
        bar.separator(Lemonbar::Separator::left, colors.first, colors.second);

        auto const relative = 100 * current_level / max_capacity;
        std::map<uint8_t, std::string> const discharging_icons = {
            {15, "\uf244"},
            {40, "\uf243"},
            {65, "\uf242"},
            {80, "\uf241"},
            {255, "\uf240"},
        };
        auto is_searched_value = [&](auto const& pair) { return pair.first > relative; };
        auto const icon = std::find_if(discharging_icons.cbegin(), discharging_icons.cend(), is_searched_value)->second;

        bar() << icon << " " << relative << "% ";
        bar.separator(Lemonbar::Separator::left);

        int64_t rem_hr_only = rem_minutes / 60;
        int64_t rem_min_only = rem_minutes % 60;
        bar() << " " << (rem_hr_only < 10 ? "0" : "") << rem_hr_only << ":" << (rem_min_only < 10 ? "0" : "")
              << rem_min_only << " ";
        break;
    }
    case Status::not_found:
        break;
    }
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
}

Battery::Battery(JSON::Node const& item) : StateItem(item), bat_file_loc(item["battery_file"].string()) {
    status = Status::not_found;
    discharge_rate = 0;
    max_capacity = 0;
    current_level = 0;
}
