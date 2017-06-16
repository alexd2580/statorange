
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <sstream>

#include "../output.hpp"
#include "../util.hpp"
#include "CPU.hpp"

using namespace std;

CPU::CPU(JSON const& item) : StateItem(item)
{
    auto& temp_paths = item["temperature_files"];
    auto num_temps = temp_paths.size();
    for(decltype(num_temps) i = 0; i < num_temps; i++)
        temp_file_paths.push_back(temp_paths[i]);

    load_file_path.assign(item["load_file"]);

    print_string = "";
    cached = false;
    cpu_temps.clear();
    cpu_load = 0.0f;
}

bool CPU::update(void)
{
    cached = false;
    char line[11];
    char* res;

    cpu_temps.clear();
    auto n = temp_file_paths.size();
    for(decltype(n) i = 0; i < n; i++)
    {
        auto path = temp_file_paths[i].c_str();
        FILE* tfile = fopen(path, "r");
        if(tfile == nullptr)
        {
            log() << "Couldn't read temperature file [" << path << "]:" << endl
                  << strerror(errno) << endl;
            cpu_temps.push_back(999);
            continue;
        }
        res = fgets(line, 10, tfile);
        fclose(tfile);
        if(res != line)
        {
            log() << "Couldn't read temperature file [" << path << "]:" << endl
                  << strerror(errno) << endl;
            cpu_temps.push_back(999);
            continue;
        }
        cpu_temps.push_back((int)strtol(line, nullptr, 0) / 1000);
    }

    FILE* lfile = fopen(load_file_path.c_str(), "r");
    if(lfile == nullptr)
    {
        log() << "Couldn't read load file [" << load_file_path << "]:" << endl
              << strerror(errno) << endl;
        cpu_load = 999.0f;
        return true;
    }
    res = fgets(line, 10, lfile);
    fclose(lfile);
    if(res != line)
    {
        log() << "Couldn't read load file [" << load_file_path << "]:" << endl
              << strerror(errno) << endl;
        cpu_load = 999.0f;
        return true;
    }
    cpu_load = strtof(line, nullptr);
    return true;
}

void CPU::print(ostream& out, uint8_t)
{
    if(!cached)
    {
        ostringstream o;
        BarWriter::dynamic_separator(
            o, BarWriter::Separator::left, cpu_load, 0.7f, 3.0f);
        o.precision(2);
        o << std::fixed << icon << ' ' << cpu_load << ' ';

        auto n = temp_file_paths.size();
        for(decltype(n) i = 0; i < n; i++)
        {
            BarWriter::dynamic_separator(
                o,
                BarWriter::Separator::left,
                (float)cpu_temps[i],
                50.0f,
                90.0f);
            o << ' ' << cpu_temps[i] << "°C ";
        }
        BarWriter::separator(
            o, BarWriter::Separator::left, BarWriter::Coloring::white_on_black);

        print_string = o.str();
        cached = true;
    }

    out << print_string;
}
