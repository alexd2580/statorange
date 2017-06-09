#include <cmath>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <sys/statvfs.h>

#include "../output.hpp"
#include "../util.hpp"
#include "Space.hpp"

using namespace std;

Space::Space(JSON const& item) : StateItem("[Space]", item)
{
    auto& mpoints = item["mount_points"];

    items.clear();
    SpaceItem si;
    for(decltype(mpoints.size()) i = 0; i < mpoints.size(); i++)
    {
        auto& mpt = mpoints[i];
        si.mount_point.assign(mpt["file"]);
        si.icon = parse_icon(mpt.get("icon").as_string_with_default(""));

        si.size = 0;
        si.used = 0;
        si.unit = "B";
        items.push_back(si);
    }
}

bool Space::getSpaceUsage(SpaceItem& dir)
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
        if(!getSpaceUsage(item))
            return false;
    return true;
}

void Space::print(void)
{
    if(items.size() > 0)
    {
        cout.precision(1);
        for(auto& item : items)
        {
            separate(Direction::left, Color::neutral);
            cout << std::fixed << item.icon << ' ' << item.mount_point << ' '
                 << item.used << '/' << item.size << item.unit << ' ';
        }
        separate(Direction::left, Color::white_on_black);
    }
}
