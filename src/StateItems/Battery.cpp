#include <utility>

#include <cstdlib>
#include <cstring>

#include "Battery.hpp"

#include "../Lemonbar.hpp"

std::pair<bool, bool> Battery::update_raw() {
    std::unique_ptr<FILE, int (*)(FILE*)> file(fopen(bat_file_loc.c_str(), "r"), fclose);
    if(file.get() == nullptr) {
        status = Status::not_found;
    } else {
        char line[201];
        while(fgets(line, 200, file.get()) != nullptr) {
            if(strncmp(line + 13, "STATUS", 6) == 0) {
                if(strncmp(line + 20, "Full", 4) == 0)
                    status = Status::full;
                else if(strncmp(line + 20, "Charging", 8) == 0)
                    status = Status::charging;
                else if(strncmp(line + 20, "Discharging", 11) == 0)
                    status = Status::discharging;
                else
                    status = Status::full;
            } else if(strncmp(line + 13, "POWER_NOW", 9) == 0)
                discharge_rate = strtol(line + 23, nullptr, 0);
            else if(strncmp(line + 13, "ENERGY_FULL_DESIGN", 18) == 0)
                max_capacity = strtol(line + 32, nullptr, 0);
            else if(strncmp(line + 13, "ENERGY_NOW", 10) == 0)
                current_level = strtol(line + 24, nullptr, 0);
        }
    }

    return {true, true};
}

void Battery::print_raw(Lemonbar& bar, uint8_t) {
    if(status == Status::not_found) {
        return;
    }

    switch(status) {
    case Status::charging:
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::neutral);
        bar() << " BAT charging " << (100 * current_level / max_capacity) << "%";
        break;
    case Status::full:
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::info);
        bar() << " BAT full";
        break;
    case Status::discharging: {
        long rem_minutes = 60 * current_level / discharge_rate;
        /*if(rem_minutes < 20) {  SEP_LEFT(warn_colors); }
        else if(rem_minutes < 60) { SEP_LEFT(info_colors); }
        else { SEP_LEFT(neutral_colors); }*/
        auto colors = Lemonbar::section_colors((float)-rem_minutes, -60.0f, -10.0f);
        bar.separator(Lemonbar::Separator::left, colors.first, colors.second);
        bar() << " BAT " << (100 * current_level / max_capacity) << "%";
        bar.separator(Lemonbar::Separator::left);

        long rem_hr_only = rem_minutes / 60;
        long rem_min_only = rem_minutes % 60;
        bar() << (rem_hr_only < 10 ? "0" : "") << rem_hr_only << ":" << (rem_min_only < 10 ? "0" : "") << rem_min_only;
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
