
#include <chrono> // chrono::system_clock
/* #include <ctime> */
#include <ostream> // ostream
#include <utility> // pair

#include "StateItems/Date.hpp"

#include "Lemonbar.hpp"
#include "utils/io.hpp"

Date::Date(JSON::Node const& item) : StateItem(item), format(item["format"].string()) {}

std::pair<bool, bool> Date::update_raw() {
    time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    std::ostringstream o;
    print_time(o, *ptm, format.c_str());
    std::string old_time(std::move(time));
    time = o.str();
    return {true, time != old_time};
}

void Date::print_raw(Lemonbar& bar, uint8_t display) {
    (void)display;
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::active);
    bar() << icon << ' ' << time << ' ';
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
}
