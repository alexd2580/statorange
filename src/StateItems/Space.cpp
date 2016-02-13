#include <iostream>
#include <string.h>
#include <sys/statvfs.h>
#include <cmath>
#include "../output.hpp"
#include "../util.hpp"
#include "Space.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

Space::Space(JSONObject& item) : StateItem(item), Logger("[Space]", cerr)
{
  JSONArray& mpoints = item["mount_points"].array();
  items.clear();
  SpaceItem si;
  for(unsigned int i = 0; i < mpoints.size(); i++)
  {
    si.mount_point = mpoints[i].string();
    si.size = "";
    si.used = "";
    items.push_back(si);
  }
}

string make2DecFloat(float f)
{
  float mp = floor(f);
  float fp = round((f - mp) * 100);
  return std::to_string((int)mp) + '.' + std::to_string((int)fp);
}

string makeHumanReadable(unsigned long bytes)
{
  if(bytes < 1024)
    return std::to_string(bytes) + "B";
  float fbytes = bytes / 1024.0f;
  if(fbytes < 1024.0f)
    return make2DecFloat(fbytes) + "KiB";
  fbytes /= 1024.0f;
  if(fbytes < 1024.0f)
    return make2DecFloat(fbytes) + "MiB";
  fbytes /= 1024.0f;
  if(fbytes < 1024.0f)
    return make2DecFloat(fbytes) + "GiB";
  fbytes /= 1024.0f;
  if(fbytes < 1024.0f)
    return make2DecFloat(fbytes) + "TiB";
  fbytes /= 1024.0f;
  if(fbytes < 1024.0f)
    return make2DecFloat(fbytes) + "EiB";
  return "TOO LARGE";
}

bool Space::getSpaceUsage(SpaceItem& dir)
{
  struct statvfs s;
  statvfs(dir.mount_point.c_str(), &s);

  unsigned long total_size = s.f_frsize * s.f_blocks;
  unsigned long used_size = total_size - s.f_bfree * s.f_bsize;

  dir.size = makeHumanReadable(total_size);
  dir.used = makeHumanReadable(used_size);
  return true;
}

bool Space::update(void)
{
  for(auto i = items.begin(); i != items.end(); i++)
    if(!getSpaceUsage(*i))
      return false;
  return true;
}

void Space::print(void)
{
  if(items.size() > 0)
  {
    separate(Left, neutral_colors);
    auto i = items.begin();
    cout << ' ' << i->mount_point << ' ' << i->used << '/' << i->size << ' ';
    for(i++; i != items.end(); i++)
    {
      separate(Left, neutral_colors);
      cout << ' ' << i->mount_point << ' ' << i->used << '/' << i->size << ' ';
    }
    separate(Left, white_on_black);
  }
}
