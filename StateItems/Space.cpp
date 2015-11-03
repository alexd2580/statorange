#include<iostream>

#include"Space.hpp"
#include"../output.hpp"
#include"../util.hpp"

using namespace std;

string const Space::getSpace = "df -h ";

/******************************************************************************/
/******************************************************************************/

void Space::getSpaceUsage(SpaceItem& dir)
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
}

void Space::performUpdate(void)
{
  for(auto i=items.begin(); i!=items.end(); i++)
    getSpaceUsage(*i);
}

Space::Space(vector<string>& mpoints) :
  StateItem(30)
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

Space::~Space()
{
}
  
void Space::print(void)
{
  separate(Left, neutral_colors);
  for(auto i=items.begin(); i!=items.end(); i++)
  {
    cout << ' ' << i->mountPoint << ' ' << i->used << '/' << i->size << ' ';
    separate(Left, neutral_colors);
  }
  separate(Left, white_on_black);
}
