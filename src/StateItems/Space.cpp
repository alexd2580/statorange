#include<iostream>

#include"Space.hpp"
#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string Space::get_space = "";

/******************************************************************************/
/******************************************************************************/

void Space::settings(JSONObject& section)
{
  get_space.assign(section["get_space"].string());
}

Space::Space(JSONObject& item) :
  StateItem(item), Logger("[Space]", cerr)
{
  /*JSONArray mpoints = item["mount_points"].array();
  mpoints.
  SpaceItem si;
  for(auto i=mpoints.begin(); i!=mpoints.end(); i++)
  {
    si.mountPoint = *i;
    si.size = "";
    si.used = "";
    items.push_back(si);
  } TODO */
}

bool Space::getSpaceUsage(SpaceItem& dir)
{
  string cmd = get_space + " " + dir.mount_point;
  string output = execute(cmd);

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
    dir.size.assign(beg, pos.ptr()-beg);
    pos.skip_whitespace();
    beg = pos.ptr();
    pos.skip_nonspace();
    dir.used.assign(beg, pos.ptr()-beg);
  }
  catch(TraceCeption& e)
  {
    log() << "While parsing output of " << get_space << endl;
    e.printStackTrace();

  }
  return true;
}

bool Space::update(void)
{
  for(auto i=items.begin(); i!=items.end(); i++)
    FAIL_ON_FALSE(getSpaceUsage(*i))
  return true;
}

void Space::print(void)
{
  if(items.size() > 0)
  {
    separate(Left, neutral_colors);
    auto i = items.begin();
    cout << ' ' << i->mount_point << ' ' << i->used << '/' << i->size << ' ';
    for(i++; i!=items.end(); i++)
    {
      separate(Left, neutral_colors);
      cout << ' ' << i->mount_point << ' ' << i->used << '/' << i->size << ' ';
    }
    separate(Left, white_on_black);
  }
}
