#include <cmath>
#include <cstring>
#include <iomanip>
#include <ostream>       // ostream
#include <sys/statvfs.h> // statvfs, struct statvfs

#include "StateItems/Space.hpp"

#include "Lemonbar.hpp"

Space::Space(JSON::Node const& item) : StateItem(item) {
    for(auto const& mpt : item["mount_points"].array()) {
        items.emplace_back(mpt["file"].string(), mpt["icon"].string(""));
    }
}

std::pair<bool, bool> Space::get_space_usage(SpaceItem& dir) {
    struct statvfs s {};
    statvfs(dir.mount_point.c_str(), &s);

    uint64_t old_size = dir.size;
    uint64_t old_used = dir.used;
    dir.size = s.f_frsize * s.f_blocks;
    dir.used = dir.size - s.f_bavail * s.f_bsize;
    return {true, dir.size != old_size || dir.used != old_used};
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

void Space::print_raw(Lemonbar& bar, uint8_t display) {
    (void)display;
    if(!items.empty()) {
        long long one_gib = 1024 * 1024 * 1024;
        for(auto& item : items) {
            auto colors = Lemonbar::section_colors(- (long long)(item.size - item.used), -50 * one_gib, -5 * one_gib);
            bar.separator(Lemonbar::Separator::left, colors.first, colors.second);
            print_used_memory(bar() << item.icon << ' ', item.used, item.size) << ' ';
        }
        bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
    }
}
