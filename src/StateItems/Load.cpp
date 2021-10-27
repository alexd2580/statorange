#include <iostream> // endl
#include <utility>  // pair

#include <cstdlib> // strtol
/* #include <cstring> */

#include <sys/sysinfo.h> // sysinfo, struct sysinfo

#include "StateItems/Load.hpp"

#include "Lemonbar.hpp"
#include "utils/convert.hpp"

bool Load::read_line(std::string const& path, char* data, uint32_t num) {
    auto file = open_file(path);
    if(file == nullptr) {
        return false;
    }
    return fgets(data, static_cast<int32_t>(num), file.get()) == data;
}

std::pair<bool, bool> Load::update_raw() {
    char line[11];
    auto line_ptr = static_cast<char*>(line);
    bool success = true;

    cpu_temps.clear();
    for(auto const& path : temp_file_paths) {
        if(read_line(path, line_ptr, 10)) {
            bool local_success;
            cpu_temps.push_back(convert_to_number<uint32_t>(line_ptr, local_success) / 1000);
        } else {
            log() << "Couldn't read temperature file [" << path << "]:" << std::endl << strerror(errno) << std::endl;
            cpu_temps.emplace_back(999);
            success = false;
        }
    }

    struct sysinfo sysinfo_data {};
    if(sysinfo(&sysinfo_data) != 0) {
        success = false;
    }

    uptime = sysinfo_data.uptime;
    constexpr float SYSINFO_LOADS_SCALE = 65536.f;
    cpu_load = static_cast<float>(sysinfo_data.loads[0]) / SYSINFO_LOADS_SCALE;
    uint32_t mem_unit = sysinfo_data.mem_unit;
    total_ram = sysinfo_data.totalram * mem_unit;
    free_ram = sysinfo_data.freeram * mem_unit;

    // This fluctuates so hard that checking whether something has changed makes no sense.
    return {success, true};
}

void Load::print_raw(Lemonbar& bar, uint8_t display) {
    (void)display;
    auto const& fire_style = Lemonbar::PowerlineStyle::fire;

    // Load.
    auto load_colors = Lemonbar::section_colors(cpu_load, 0.8f, 2.2f);
    bar.separator(Lemonbar::Separator::left, load_colors.first, load_colors.second, fire_style);
    bar().precision(2);
    bar() << std::fixed << icon << ' ' << cpu_load << ' ';

    // RAM usage.
    auto neg_free_ram = -static_cast<int64_t>(free_ram);
    auto neg_half_ram = -static_cast<int64_t>(total_ram) / 2;
    auto memory_colors = Lemonbar::section_colors(neg_free_ram, neg_half_ram, 0L);
    bar.separator(Lemonbar::Separator::left, memory_colors.first, memory_colors.second, fire_style);
    print_used_memory(bar() << ' ', total_ram - free_ram, total_ram) << ' ';

    bool is_first = true;
    for(uint32_t temp : cpu_temps) {
        auto style = is_first ? fire_style : Lemonbar::PowerlineStyle::none;
        is_first = false;
        auto temp_colors = Lemonbar::section_colors<uint32_t>(temp, 50, 90);
        bar.separator(Lemonbar::Separator::left, temp_colors.first, temp_colors.second, style);
        bar() << ' ' << temp << "\ufa03 ";
    }
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black, fire_style);
}

Load::Load(JSON::Node const& item) : StateItem(item) {
    auto const temp_paths = item["temperature_files"].array();
    for(auto const& temp_path : temp_paths) {
        temp_file_paths.push_back(temp_path.string());
    }
    cpu_temps.clear();
    cpu_load = 0.0f;
    uptime = 0;
    total_ram = 0;
    free_ram = 0;
}
