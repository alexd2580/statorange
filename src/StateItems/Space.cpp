#include <iostream>

#include "../output.hpp"
#include "../util.hpp"
#include "Space.hpp"

using namespace std;

string Space::get_space = "";

/******************************************************************************/
/******************************************************************************/

void Space::settings(JSONObject& section)
{
  get_space.assign(section["get_space"].string());
}

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

bool Space::getSpaceUsage(SpaceItem& dir)
{
  string cmd = get_space + " " + dir.mount_point;
  string output;
  bool yay = false; // broken architecture.
  if(!execute(cmd, output, yay))
  {
    log() << "Couldn't execute " << cmd << endl;
    return false;
  }

  try
  {
    TextPos pos(output.c_str());
    while(*pos != '\n')
      (void)pos.next();
    (void)pos.next();
    pos.skip_nonspace();
    pos.skip_whitespace();
    char const* beg = pos.ptr();
    pos.skip_nonspace();
    dir.size.assign(beg, (size_t)(pos.ptr() - beg));
    pos.skip_whitespace();
    beg = pos.ptr();
    pos.skip_nonspace();
    dir.used.assign(beg, (size_t)(pos.ptr() - beg));
  }
  catch(TraceCeption& e)
  {
    log() << "While parsing output of " << get_space << endl;
    e.printStackTrace();
    return false;
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
