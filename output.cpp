#include<stdio.h>
#include<string.h>

#include"output.h"
#include"config.h"

/******************************************************************************/
/*****************************     ICONS     **********************************/

#define PRINT_ICON(icon) \
  printf("%%{T3}%s%%{T1}", icon)

// Icon glyphs from Terminusicons2
char const* icon_clock="Õ";          // Clock icon
char const* icon_cpu="Ï";            // CPU icon
char const* icon_mem="Þ";            // MEM icon
char const* icon_dl="Ð";             // Download icon
char const* icon_ul="Ñ";             // Upload icon
char const* icon_vol="Ô";            // Volume icon
char const* icon_hd="À";             // HD / icon
char const* icon_home="Æ";           // HD /home icon
char const* icon_mail="Ó";           // Mail icon
char const* icon_chat="Ò";           // IRC/Chat icon
char const* icon_music="Î";          // Music icon
char const* icon_prog="Â";           // Window icon
char const* icon_contact="Á";        // Contact icon
char const* icon_wsp="É";            // Workspace icon
char const* icon_wlan="Ø";           // WIFI icon 

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

char const* color_white = "white";
char const* color_dwhite = "#FFCCCCCC";
char const* color_lgrey = "#FF707070";
char const* color_grey = "#FF454545";
char const* color_dgrey = "#FF2A2A2A";
char const* color_red = "red";
char const* color_blue = "#FF1010D0";
char const* color_green = "#FF10D010";
char const* color_dgreen = "#FF008000";
char const* color_yellow = "#FFCDCD00";
char const* color_blind = "#FF8B814C";
char const* color_crimson = "#FFDC143C";
char const* color_black = "black";

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

char const** current_set_colors = white_on_black;

#define SEP_RIGHT(colors) { \
  if(colors[0] != current_set_colors[0]) \
    printf("%%{R}%%{B%s}%s%%{F%s}", colors[0], sep_right, colors[1]); \
  else \
    printf("%%{Fblack}%s%%{F%s}", sep_l_right, colors[1]); \
  current_set_colors = colors; }

#define SEP_LEFT(colors) { \
  if(colors[0] != current_set_colors[0]) \
    printf("%%{F%s}%s%%{R}%%{F%s}", colors[0], sep_left, colors[1]); \
  else \
    printf("%%{Fblack}%s%%{F%s}", sep_l_left, colors[1]); \
  current_set_colors = colors; }

void dynamic_section(float value, float min, float max)
{
  if(value <= min)
    SEP_LEFT(neutral_colors)
  else if(value >= max)
    SEP_LEFT(warn_colors)
  else
  {
    char back_1[9];
    char back_2[9];
    char* back;
    back = current_set_colors[0] == back_1 ? back_2 : back_1;
      
    uint8_t byteV = (uint8_t)((value-min)*256.0f / (max-min));
    MAKE_HEX_COLOR(back, 255, byteV, (uint8_t)(127-byteV/2), 0)
    char const* colors[2] = { back, color_dwhite };
    SEP_LEFT(colors)
  }
}

/******************************************************************************/
/****************************     OUTPUT     **********************************/

/**
 * Prints the buttons for switching workspaces
 * align them to the left side
 */
void echoWorkspaceButtons(I3State& i3, Output* disp)
{
  Workspace* ws;
  for(size_t w=0; w<i3->wsCount; w++)
  {
    ws = i3->workspaces+w;
    if(ws->output == disp)
    {
      char const** colors = inactive_colors;
      if(ws->urgent) colors = warn_colors;
      else if(ws->focused) colors = active_colors;
      else if(ws->visible) colors = semiactive_colors;
    
      printf("%%{A:i3-msg workspace ");
      char* c = ws->name;
      while(*c != '\0')
      {
        if(*c == ':') printf("\\:");
        else putchar(*c);
        c++;
      }
      printf(":}");
      SEP_RIGHT(colors);
      printf(" %s ", ws->name);
      if(ws->visible && ws->focusedApp[0] != '\0')
      {
        SEP_RIGHT(colors);
        printf(" %s ", ws->focusedApp);
      }
      printf("%%{A}");
    }
  }
  SEP_RIGHT(white_on_black);
}

/**
 * Prints the input to lemonbar for the main monitor
 * TODO specify main monitor in config -> fix main, not this method 
 */
void echoPrimaryLemon(I3State& i3, SystemState& sys, Output* disp, uint8_t pos)
{
  cout << "%%{S" << pos << "}";
  cout << "%%{l}";
  echoWorkspaceButtons(i3, disp);
  if(i3.mode.compare("default") != 0)
  {
    cout << ' ';
    SEP_LEFT(info_colors);
    cout << ' ' << i3.mode << ' ';
    SEP_RIGHT(white_on_black);
  }
  cout << "%%{l}";
  cout << "%%{r}";
  
  if(sys->net_eth_up)
  {
    SEP_LEFT(neutral_colors);
    cout << ' ' << ethernet << ' ' << sys.net_eth_ip << ' ';
    SEP_LEFT(white_on_black);
  }
  
  if(sys->net_wlan_up)
  {
    SEP_LEFT(neutral_colors);
    PRINT_ICON(icon_wlan);
    cout << ' ' << sys.net_wlan_essid << "(" << sys.net_wlan_quality << "%%) ";
    SEP_LEFT(neutral_colors);
    cout << ' ' << sys.net_wlan_ip << ' ';
    SEP_LEFT(white_on_black);
  }
  
  if(sys->bat_status != NO_BATTERY)
  {
    switch(sys->bat_status)
    {
    case BATTERY_CHARGING:
      SEP_LEFT(neutral_colors);
      cout << " BAT charging " << 100*sys->bat_level/sys->bat_capacity << "%% ";
      break;
    case BATTERY_FULL:
      SEP_LEFT(info_colors);
      cout << " BAT full ";
      break;
    case BATTERY_DISCHARGING:
    {
      long rem_minutes = 60*sys->bat_level / sys->bat_discharge;
      /*if(rem_minutes < 20) {  SEP_LEFT(warn_colors); }
      else if(rem_minutes < 60) { SEP_LEFT(info_colors); }
      else { SEP_LEFT(neutral_colors); }*/
      dynamic_section((float)-rem_minutes, -60.0f, -10.0f);
      cout << " BAT " << 100*sys.bat_level/sys.bat_capacity << "%% ";
      SEP_LEFT(current_set_colors);

      long rem_hrOnly = rem_minutes / 60;
      if(rem_hrOnly < 10) cout << " 0" << rem_hrOnly;
      else cout << ' ' << rem_hrOnly;

      long rem_minOnly = rem_minutes % 60;
      if(rem_minOnly < 10) cout << ":0" << rem_minOnly;
      else cout << ':' << rem_minOnly;
      
      break;
    }
    default:
      break;
    }
    SEP_LEFT(white_on_black)
  }
  
  /*if(sys->cpu_load > 3.0f) { SEP_LEFT(warn_colors); }
  else if(sys->cpu_load > 1.0f) { SEP_LEFT(info_colors); }
  else { SEP_LEFT(neutral_colors); }*/
  dynamic_section(sys.cpu_load, 0.7f, 3.0f);
  PRINT_ICON(icon_cpu);
  printf(" %.2f ", (double)sys.cpu_load);
  
  /*if(sys->cpu_temp >= 70) { SEP_LEFT(warn_colors); }
  else if(sys->cpu_temp >= 55) { SEP_LEFT(info_colors); }
  else { SEP_LEFT(neutral_colors); }*/
  dynamic_section((float)sys.cpu_temp, 50.0f, 90.0f);
  cout << ' ' << sys.cpu_temp << "°C ";
  SEP_LEFT(white_on_black);

  SEP_LEFT(active_colors);
  PRINT_ICON(icon_clock);
  cout << ' ' << sys.time << ' ';
  SEP_LEFT(white_on_black);

  cout << "%%{r}";
  cout << "%%{S" << pos << "}";
  return;
}

/**
 * Prints the input to lemonbar for a secondary monitor
 */
void echoSecondaryLemon(I3State& i3, SystemState& sys, Output* disp, uint8_t pos)
{
  (void)sys;
  cout << "%%{S" << pos << "}";
  cout << "%%{l}";
  echoWorkspaceButtons(i3, disp);
  cout << "%%{l}";
  cout << "%%{r}";
  cout << "%%{r}";
  cout << "%%{S" << pos << "}";
  return;
}

/* ************************************************************************** */
