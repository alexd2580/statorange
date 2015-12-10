#include<cstring>
#include<string>

#include"output.hpp"
#include"StateItem.hpp"

using namespace std;

/******************************************************************************/
/*****************************     ICONS     **********************************/

// Icon glyphs from Terminusicons2
char const* const icon_clock="Õ";          // Clock icon
char const* const icon_cpu="Ï";            // CPU icon
char const* const icon_mem="Þ";            // MEM icon
char const* const icon_dl="Ð";             // Download icon
char const* const icon_ul="Ñ";             // Upload icon
char const* const icon_vol="Ô";            // Volume icon
char const* const icon_hd="À";             // HD / icon
char const* const icon_home="Æ";           // HD /home icon
char const* const icon_mail="Ó";           // Mail icon
char const* const icon_chat="Ò";           // IRC/Chat icon
char const* const icon_music="Î";          // Music icon
char const* const icon_prog="Â";           // Window icon
char const* const icon_contact="Á";        // Contact icon
char const* const icon_wsp="É";            // Workspace icon
char const* const icon_wlan="Ø";           // WIFI icon 

/******************************************************************************/
/*****************************     COLORS     *********************************/

char const* hex_color = "0123456789ABCDEF";

void make_hex(char* dst, uint8_t a)
{
  dst[0] = hex_color[a / 16];
  dst[1] = hex_color[a % 16];
}

#define MAKE_HEX_COLOR(color, a, r, g, b) { \
  color[0] = '#'; \
  make_hex(color+1, a); \
  make_hex(color+3, r); \
  make_hex(color+5, g); \
  make_hex(color+7, b); }

char const* color_white = "#FFFFFFFF";
char const* color_dwhite = "#FFCCCCCC";
char const* color_lgrey = "#FF707070";
char const* color_grey = "#FF454545";
char const* color_dgrey = "#FF2A2A2A";
char const* color_red = "#FFFF0000";
char const* color_blue = "#FF1010D0";
char const* color_green = "#FF10D010";
char const* color_dgreen = "#FF008000";
char const* color_yellow = "#FFCDCD00";
char const* color_blind = "#FF8B814C";
char const* color_crimson = "#FFDC143C";
char const* color_black = "#FF000000";

char const* white_on_black[2]; //{ color_black, color_white };
char const* inactive_colors[2]; //{ color_dgrey, color_lgrey };
char const* semiactive_colors[2]; //{ color_grey, color_dwhite };
char const* active_colors[2]; //{ color_blue, color_dwhite };
char const* warn_colors[2]; //{ color_red, color_dwhite };
char const* info_colors[2]; //{ color_yellow, color_black };
char const* good_colors[2]; //{ color_green, color_black };
char const* neutral_colors[2]; //{ color_dgreen, color_black };

void init_colors(void)
{
  white_on_black[0] = color_black; white_on_black[1] = color_white;
  inactive_colors[0] = color_dgrey; inactive_colors[1] = color_lgrey;
  semiactive_colors[0] = color_grey; semiactive_colors[1] = color_dwhite;
  active_colors[0] = color_blue; active_colors[1] = color_dwhite;
  warn_colors[0] = color_red; warn_colors[1] = color_dwhite;
  info_colors[0] = color_yellow; info_colors[1] = color_black;
  good_colors[0] = color_green; good_colors[1] = color_black;
  neutral_colors[0] = color_dgreen; neutral_colors[1] = color_dwhite;
  return;
}

/******************************************************************************/
/**************************     SEPARATORS     ********************************/

// Char glyps for powerline fonts
char const* sep_left="";           // Powerline separator left
char const* sep_right="";          // Powerline separator right
char const* sep_l_left="";         // Powerline light separator left
char const* sep_l_right="";        // Powerline light sepatator right

void separate(Direction d, char const** colors, ostream& o)
{
  static char const** current_set_colors = white_on_black;

  if(colors == nullptr)
    colors = current_set_colors;

  if(d == Left)
  {
    if(colors[0] != current_set_colors[0])
      o << "%{F" << colors[0] << '}' << sep_left << "%{R}%{F" << colors[1] << '}';
    else
      o << "%{F#FF000000}" << sep_l_left << "%{F" << colors[1] << '}';
  }
  else
  {
    if(colors[0] != current_set_colors[0])
      o << "%{R}%{B" << colors[0] << '}' << sep_right << "%{F" << colors[1] << '}';
    else
      o << "%{F#FF000000}" << sep_l_left << "%{F" << colors[1] << '}';
  }
  current_set_colors = colors;
}


void dynamic_section(float value, float min, float max, ostream& o)
{
  if(value <= min)
    separate(Left, neutral_colors, o);
  else if(value >= max)
    separate(Left, warn_colors, o);
  else
  {
    static char back_1[9];
    static char back_2[9];
    static char* back = back_1;
    back = (char*)((long)back ^ (long)back_1 ^ (long)back_2);
      
    uint8_t byteV = (uint8_t)((value-min)*256.0f / (max-min));
    MAKE_HEX_COLOR(back, 255, byteV, (uint8_t)(127-byteV/2), 0)
    static char const* colors[2];
    colors[0] = back;
    colors[1] = color_dwhite;
    separate(Left, colors, o);
  }
}

/******************************************************************************/
/***************************     BUTTONS     **********************************/

void startButton(string cmd, ostream& o)
{
  o << "%{A:";
  for(auto i=cmd.begin(); i!=cmd.end(); i++)
    o << (*i == ':' ? "\\" : "") << *i;
  o << ":}";
}

void stopButton(ostream& o)
{
  o << "%{A}";
}

/******************************************************************************/
/****************************     OUTPUT     **********************************/

/**
 * Prints the buttons for switching workspaces
 * align them to the left side
 */
void echoWorkspaceButtons(I3State& i3, uint8_t disp)
{
  for(size_t w=0; w<i3.workspaces.size(); w++)
  {
    Workspace& ws = i3.workspaces[w];
    if(ws.output == disp)
    {
      char const** colors = inactive_colors;
      if(ws.urgent) colors = warn_colors;
      else if(ws.focused) colors = active_colors;
      else if(ws.visible) colors = semiactive_colors;
    
      startButton("i3-msg workspace " + ws.name);
      separate(Right, colors);
      cout << ' ' << ws.name << ' ';
      if(ws.focusedApp.size() != 0)
      {
        separate(Right, colors);
        cout << ' ' << ws.focusedApp << ' ';
      }
      separate(Right, white_on_black);
      stopButton();
    }
  }
}

/**
 * Prints the input to lemonbar for the main monitor
 * TODO specify main monitor in config -> fix main, not this method 
 */
void echoPrimaryLemon(I3State& i3, uint8_t disp)
{
  cout << "%{S" << (int)disp << "}";
  cout << "%{l}";
  echoWorkspaceButtons(i3, disp);
  if(i3.mode.compare("default") != 0)
  {
    cout << ' ';
    separate(Left, info_colors);
    cout << ' ' << i3.mode << ' ';
    separate(Right, white_on_black);
  }
  cout << "%{l}";
  cout << "%{r}";
  
	StateItem::printState();

  cout << "%{r}";
  cout << "%{S" << (int)disp << "}";
  return;
}

/**
 * Prints the input to lemonbar for a secondary monitor
 */
void echoSecondaryLemon(I3State& i3, uint8_t disp)
{
  cout << "%{S" << (int)disp << "}";
  cout << "%{l}";
  echoWorkspaceButtons(i3, disp);
  cout << "%{l}";
  cout << "%{r}";
  cout << "%{r}";
  cout << "%{S" << (int)disp << "}";
  return;
}

/* ************************************************************************** */
