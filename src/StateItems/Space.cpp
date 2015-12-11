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
  StateItem(item)
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

  char const* c = output.c_str();
  while(*c != '\n') c++;
  c++;
  skipNonWhitespace(c);
  skipWhitespaces(c);
  char const* beg = c;
  skipNonWhitespace(c);
  dir.size.assign(beg, c-beg);
  skipWhitespaces(c);
  beg = c;
  skipNonWhitespace(c);
  dir.used.assign(beg, c-beg);

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
