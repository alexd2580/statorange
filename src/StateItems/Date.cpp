
#include <chrono> // std::chrono::system_clock
#include <ctime>
#include <iomanip> // std::put_time
#include <ostream>
#include <sstream>
#include <utility> // std::pair

#include "Date.hpp"

#include "../Lemonbar.hpp"
#include "../util.hpp"

Date::Date(JSON::Node const& item) : StateItem(item), format(item["format"].string()) {}

std::pair<bool, bool> Date::update_raw() {
    time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    std::ostringstream o;
    print_time(o, *ptm, format.c_str());
    const bool changed = time != (time = o.str());
    return {true, changed};
}

void Date::print_raw(Lemonbar& bar, uint8_t) {
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::active);
    bar() << icon << ' ' << time << ' ';
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
}
