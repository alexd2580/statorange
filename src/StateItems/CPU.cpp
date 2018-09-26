#include <cstdlib>
#include <cstring>
#include <ostream>

#include "StateItems/CPU.hpp"

#include "Lemonbar.hpp"
#include "util.hpp"

bool CPU::read_line(std::string const& path, char* data, uint32_t num) {
    std::unique_ptr<FILE, int (*)(FILE*)> file(fopen(path.c_str(), "re"), fclose);
    if(file == nullptr) {
        return false;
    }
    return fgets(data, static_cast<int32_t>(num), file.get()) == data;
}

std::pair<bool, bool> CPU::update_raw() {
    char line[11];
    auto line_ptr = static_cast<char*>(line);
    bool success = true;

    cpu_temps.clear();
    for(auto const& path : temp_file_paths) {
        if(read_line(path, line_ptr, 10)) {
            cpu_temps.push_back(static_cast<uint32_t>(strtol(line_ptr, nullptr, 0) / 1000));
        } else {
            log() << "Couldn't read temperature file [" << path << "]:" << std::endl << strerror(errno) << std::endl;
            cpu_temps.emplace_back(999);
            success = false;
        }
    }

    if(read_line(load_file_path, line_ptr, 10)) {
        cpu_load = strtof(line_ptr, nullptr);
    } else {
        log() << "Couldn't read load file [" << load_file_path << "]:" << std::endl << strerror(errno) << std::endl;
        cpu_load = 999.0f;
        success = false;
    }
    // This fluctuates so hard that checking whether something has changed makes no sense.
    return {success, true};
}

void CPU::print_raw(Lemonbar& bar, uint8_t) {
    auto load_colors = Lemonbar::section_colors(cpu_load, 0.7f, 3.0f);
    bar.separator(Lemonbar::Separator::left, load_colors.first, load_colors.second);
    bar().precision(2);
    bar() << std::fixed << icon << ' ' << cpu_load << ' ';

    for(uint32_t temp : cpu_temps) {
        auto temp_colors = Lemonbar::section_colors<uint32_t>(temp, 50, 90);
        bar.separator(Lemonbar::Separator::left, temp_colors.first, temp_colors.second);
        bar() << ' ' << temp << "°C ";
    }
    bar.separator(Lemonbar::Separator::left, Lemonbar::Coloring::white_on_black);
}

CPU::CPU(JSON::Node const& item) : StateItem(item), load_file_path(item["load_file"].string()) {
    auto const temp_paths = item["temperature_files"].array();
    for(auto const& temp_path : temp_paths) {
        temp_file_paths.push_back(temp_path.string());
    }
    cpu_temps.clear();
    cpu_load = 0.0f;
}
