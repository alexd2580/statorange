#include<iostream>
#include<cstring>

#include"Volume.hpp"
#include"../output.hpp"
#include"../util.hpp"

using namespace std;

/******************************************************************************/
/******************************************************************************/

void Volume::performUpdate(void)
{
  string output = execute(amixer_cmd);
  char const* c = output.c_str();

  while(*c != '\0')
  {
    int unused;
    char on[5] = { 0 };
    int matched = sscanf(c, "  Front Left: Playback %d [%d%%] [%s", &unused, &volume, on);
    if(matched == 3)
    {
      mute = strncmp(on, "on]", 3) != 0;
      return;
    }
    
    while(*c != '\n' && *c != '\0')
      c++;
    if(*c == '\n')
      c++;
  }
}

Volume::Volume() :
  StateItem(300),
  amixer_cmd("amixer get Master"),
  alsamixer_cmd(mkTerminalCmd("alsamixer"))
{
}

void Volume::print(void)
{
  startButton(alsamixer_cmd);
  separate(Left, neutral_colors);
  PRINT_ICON(icon_vol);
  if(mute)
    cout << " Mute ";
  else
    cout << ' ' << volume << "% ";
  separate(Left, white_on_black);
  stopButton();
}
