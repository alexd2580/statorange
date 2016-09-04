#include <cmath>
#include <iostream>
#include <string.h>
#include <sys/statvfs.h>

#include "../output.hpp"
#include "../util.hpp"
#include "../JSON/JSONException.hpp"
#include "Space.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

Space::Space(JSON const& item) : StateItem(item), Logger("[Space]")
{
  auto& mpoints = item["mount_points"];

  items.clear();
  SpaceItem si;
  for(decltype(mpoints.size()) i = 0; i < mpoints.size(); i++)
  {
    auto& mpt = mpoints[i];
    si.mount_point.assign(mpt["file"]);
    try
    {
      si.icon = parse_icon(mpt["icon"]);
    }
    catch(JSONException&)
    {
      si.icon = Icon::no_icon;
    }

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
  for(auto i = items.begin(); i != items.end(); i++)
    if(!getSpaceUsage(*i))
      return false;
  return true;
}

#include <iomanip>

void Space::print(void)
{
  if(items.size() > 0)
  {
    separate(Direction::left, Color::neutral);
    auto i = items.begin();
    cout.precision(1);
    cout << std::fixed << i->icon << ' ' << i->mount_point << ' ' << i->used
         << '/' << i->size << i->unit << ' ';
    for(i++; i != items.end(); i++)
    {
      separate(Direction::left, Color::neutral);
      cout << std::fixed << i->icon << ' ' << i->mount_point << ' ' << i->used
           << '/' << i->size << i->unit << ' ';
    }
    separate(Direction::left, Color::white_on_black);
  }
}
