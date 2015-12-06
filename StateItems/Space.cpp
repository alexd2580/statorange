#include<iostream>

#include"Space.hpp"
#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string const Space::getSpace = "df -h";

/******************************************************************************/
/******************************************************************************/

bool Space::getSpaceUsage(SpaceItem& dir)
{
  string cmd = getSpace + " " + dir.mountPoint;
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

Space::Space(vector<string>& mpoints) :
  StateItem("SpaceUsage", 30)
{
  SpaceItem si;
  for(auto i=mpoints.begin(); i!=mpoints.end(); i++)
  {
    si.mountPoint = *i;
    si.size = "";
    si.used = "";
    items.push_back(si);
  }
}

void Space::print(void)
{
  if(items.size() > 0)
  {
    separate(Left, neutral_colors);
    auto i = items.begin();
    cout << ' ' << i->mountPoint << ' ' << i->used << '/' << i->size << ' ';
    for(i++; i!=items.end(); i++)
    {
      separate(Left, neutral_colors);
      cout << ' ' << i->mountPoint << ' ' << i->used << '/' << i->size << ' ';
    }
    separate(Left, white_on_black);
  }
}
