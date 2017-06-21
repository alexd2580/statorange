
#include <cstdlib>
#include <cstring>

#include "../output.hpp"
#include "Battery.hpp"

using namespace std;

Battery::Battery(JSON const& item) : StateItem(item)
{
    bat_file_loc.assign(item["battery_file"]);

    status = BatStatus::not_found;
    discharge_rate = 0;
    max_capacity = 0;
    current_level = 0;
}

bool Battery::update(void)
{
    FILE* bfile = fopen(bat_file_loc.c_str(), "r");
    if(bfile == nullptr)
        status = BatStatus::not_found;
    else
    {
        char line[201];
        while(fgets(line, 200, bfile) != nullptr)
        {
            if(strncmp(line + 13, "STATUS", 6) == 0)
            {
                if(strncmp(line + 20, "Full", 4) == 0)
                    status = BatStatus::full;
                else if(strncmp(line + 20, "Charging", 8) == 0)
                    status = BatStatus::charging;
                else if(strncmp(line + 20, "Discharging", 11) == 0)
                    status = BatStatus::discharging;
                else
                    status = BatStatus::full;
            }
            else if(strncmp(line + 13, "POWER_NOW", 9) == 0)
                discharge_rate = strtol(line + 23, nullptr, 0);
            else if(strncmp(line + 13, "ENERGY_FULL_DESIGN", 18) == 0)
                max_capacity = strtol(line + 32, nullptr, 0);
            else if(strncmp(line + 13, "ENERGY_NOW", 10) == 0)
                current_level = strtol(line + 24, nullptr, 0);
        }
        fclose(bfile);
    }

    return true;
}

void Battery::print(ostream& out, uint8_t)
{
    if(status != BatStatus::not_found)
    {
        switch(status)
        {
        case BatStatus::charging:
            BarWriter::separator(
                out, BarWriter::Separator::left, BarWriter::Coloring::neutral);
            out << " BAT charging " << 100 * current_level / max_capacity
                << "% ";
            break;
        case BatStatus::full:
            BarWriter::separator(
                out, BarWriter::Separator::left, BarWriter::Coloring::info);
            out << " BAT full ";
            break;
        case BatStatus::discharging:
        {
            long rem_minutes = 60 * current_level / discharge_rate;
            /*if(rem_minutes < 20) {  SEP_LEFT(warn_colors); }
            else if(rem_minutes < 60) { SEP_LEFT(info_colors); }
            else { SEP_LEFT(neutral_colors); }*/
            auto colors =
                BarWriter::section_colors((float)-rem_minutes, -60.0f, -10.0f);
            BarWriter::separator(out, BarWriter::Separator::left, colors);
            out << " BAT " << (int)(100 * current_level / max_capacity) << "% ";
            BarWriter::separator(out, BarWriter::Separator::left);

            long rem_hr_only = rem_minutes / 60;
            out << (rem_hr_only < 10 ? " 0" : " ") << rem_hr_only;

            long rem_min_only = rem_minutes % 60;
            out << (rem_min_only < 10 ? ":0" : ":") << rem_min_only << ' ';

            break;
        }
        case BatStatus::not_found:
            break;
        }
        BarWriter::separator(
            out,
            BarWriter::Separator::left,
            BarWriter::Coloring::white_on_black);
    }
}
