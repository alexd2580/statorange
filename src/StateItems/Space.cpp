#include <cmath>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <sys/statvfs.h>

#include "Space.hpp"

#include "../Lemonbar.hpp"
#include "../util.hpp"

std::ostream& operator<<(std::ostream& out, SpaceItem const& item) {
    out << item.icon << ' ' << item.mount_point << ' ';
    constexpr uint64_t cutoff = 1200;
    uint8_t unit = 0;
    uint64_t unit_factor = 1;
    while(item.size > cutoff * unit_factor) {
        unit++;
        unit_factor *= 1024;
    }
    std::string const unit_str(unit < 2 ? (unit == 0 ? "B" : "KiB")
                                        : unit < 4 ? (unit == 2 ? "MiB" : "GiB") : (unit == 4 ? "TiB" : "PiB"));

    return out << (item.used / unit_factor) << '.' << ((item.used % unit_factor) / (unit_factor / 10)) << '/'
               << (item.size / unit_factor) << '.' << ((item.size % unit_factor) / (unit_factor / 10)) << unit_str;
}

Space::Space(JSON::Node const& item) : StateItem(item) {
    for(auto const mpt : item["mount_points"].array()) {
        items.emplace_back(mpt["file"].string(), Lemonbar::parse_icon(mpt["icon"].string()));
    }
}

std::pair<bool, bool> Space::get_space_usage(SpaceItem& dir) {
    struct statvfs s;
    statvfs(dir.mount_point.c_str(), &s);

    bool changed = false;
    changed = dir.size != (dir.size = s.f_frsize * s.f_blocks) || changed;
    changed = dir.used != (dir.used = dir.size - s.f_bavail * s.f_bsize) || changed;
    return {true, changed};
}

std::pair<bool, bool> Space::update_raw() {
    bool valid = true;
    bool changed = false;
    for(auto& item : items) {
        auto item_status = get_space_usage(item);
        valid = item_status.first && valid;
        changed = item_status.second || changed;
    }
    return {valid, changed};
}

void Space::print_raw(Lemonbar& bar, uint8_t) {
    if(!items.empty()) {
        for(auto& item : items) {
            bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::neutral);
            bar() << item << ' ';
        }
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
    }
}
