#include<iostream>
#include<cstring>

#include"Volume.hpp"
#include"../output.hpp"
#include"../util.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

string Volume::get_volume = "";

void Volume::settings(JSONObject& section)
{
    string s = section["get_volume"].string();
    get_volume.assign(s);
}

Volume::Volume(JSONObject& item) :
  StateItem(item)
{
    mute = true;
    volume = 0;
}

bool Volume::update(void)
{
  string output = execute(get_volume);
  char const* c = output.c_str();

  while(*c != '\0')
  {
    int unused;
    char on[5] = { 0 };
    int matched = sscanf(c, "  Front Left: Playback %d [%d%%] [%s", &unused, &volume, on);
    // TODO other formats
    if(matched == 3)
    {
      mute = strncmp(on, "on]", 3) != 0;
      return true;
    }

    while(*c != '\n' && *c != '\0')
      c++;
    if(*c == '\n')
      c++;
  }

  return false;
}

void Volume::print(void)
{
  startButton(get_volume);
  separate(Left, neutral_colors);
  print_icon(icon_vol);
  if(mute)
    cout << " Mute ";
  else
    cout << ' ' << volume << "% ";
  separate(Left, white_on_black);
  stopButton();
}
