#include <cmath>
#include <iomanip>
#include <ostream>
#include <string.h>
#include <sys/statvfs.h>

#include "../output.hpp"
#include "../util.hpp"
#include "Space.hpp"

using namespace std;

Space::Space(JSON const& item) : StateItem(item)
{
    auto& mpoints = item["mount_points"];

    items.clear();
    SpaceItem si;
    for(decltype(mpoints.size()) i = 0; i < mpoints.size(); i++)
    {
        auto& mpt = mpoints[i];
        si.mount_point.assign(mpt["file"]);
        si.icon =
            BarWriter::parse_icon(mpt.get("icon").as_string_with_default(""));

        si.size = 0;
        si.used = 0;
        si.unit = "B";
        items.push_back(si);
    }
}

bool Space::get_space_usage(SpaceItem& dir)
{
    struct statvfs s;
    statvfs(dir.mount_point.c_str(), &s);

    dir.size = s.f_frsize * s.f_blocks;
    dir.used = dir.size - s.f_bfree * s.f_bsize;

    unsigned short u = 0;
    while(dir.size > 1024.0f && u < 5)
    {
        dir.used /= 1024.0f;
        dir.size /= 1024.0f;
        u++;
    }

    switch(u)
    {
    case 0:
        dir.unit = "B";
        break;
    case 1:
        dir.unit = "KiB";
        break;
    case 2:
        dir.unit = "MiB";
        break;
    case 3:
        dir.unit = "GiB";
        break;
    case 4:
        dir.unit = "TiB";
        break;
    default:
        dir.unit = "PiB";
        break;
    }
    return true;
}

bool Space::update(void)
{
    for(auto& item : items)
        if(!get_space_usage(item))
            return false;
    return true;
}

void Space::print(ostream& out, uint8_t)
{
    if(items.size() > 0)
    {
        out.precision(1);
        for(auto& item : items)
        {
            BarWriter::separator(
                out, BarWriter::Separator::left, BarWriter::Coloring::neutral);
            out << std::fixed << item.icon << ' ' << item.mount_point << ' '
                << item.used << '/' << item.size << item.unit << ' ';
        }
        BarWriter::separator(
            out,
            BarWriter::Separator::left,
            BarWriter::Coloring::white_on_black);
    }
}
