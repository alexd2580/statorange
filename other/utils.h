// Icon glyphs from Terminusicons2
char* icon_clock="Õ";          // Clock icon
char* icon_cpu="Ï";            // CPU icon
char* icon_mem="Þ";            // MEM icon
char* icon_dl="Ð";             // Download icon
char* icon_ul="Ñ";             // Upload icon
char* icon_vol="Ô";            // Volume icon
char* icon_hd="À";             // HD / icon
char* icon_home="Æ";           // HD /home icon
char* icon_mail="Ó";           // Mail icon
char* icon_chat="Ò";           // IRC/Chat icon
char* icon_music="Î";          // Music icon
char* icon_prog="Â";           // Window icon
char* icon_contact="Á";        // Contact icon
char* icon_wsp="É";            // Workspace icon
char* icon_wlan="Ø";           // WIFI icon 

#define PRINT_ICON(icon) \
  printf("%%{T3}%s%%{T1}", icon)
  
char* color_white = "white";
char* color_dwhite = "#FFCCCCCC";
char* color_lgrey = "#FF707070";
char* color_grey = "#FF454545";
char* color_dgrey = "#FF2A2A2A";
char* color_dimmred = "#FFD01010";
char* color_dimmblue = "#FF1010D0";
char* color_black = "black";

char* white_on_black[2]; //{ color_black, color_white };
char* normal_colors[2]; //{ color_dgrey, color_lgrey };
char* active_colors[2]; //{ color_dimmblue, color_dwhite };
char* visible_colors[2]; //{ color_grey, color_dwhite };
char* hl_colors[2]; //{ color_dimmred, color_dwhite };

void init_colors(void)
{
  white_on_black[0] = color_black; white_on_black[1] = color_white;
  normal_colors[0] = color_dgrey; normal_colors[1] = color_lgrey;
  active_colors[0] = color_dimmblue; active_colors[1] = color_dwhite;
  visible_colors[0] = color_grey; visible_colors[1] = color_dwhite;
  hl_colors[0] = color_dimmred; hl_colors[1] = color_dwhite;
  return;
}

// Char glyps for powerline fonts
char* sep_left="";           // Powerline separator left
char* sep_right="";          // Powerline separator right
char* sep_l_left="";         // Powerline light separator left
char* sep_l_right="";        // Powerline light sepatator right

char** current_set_colors = white_on_black;

#define SEP_RIGHT(colors) \
  if(colors[0] != current_set_colors[0]) \
    printf("%%{R}%%{B%s}%s%%{F%s}", colors[0], sep_right, colors[1]); \
  else printf("%%{Fblack}%s%%{F%s}", sep_l_right, colors[1]); \
  current_set_colors = colors;

#define SEP_LEFT(colors) \
  if(colors[0] != current_set_colors[0]) \
    printf("%%{F%s}%s%%{R}%%{F%s}", colors[0], sep_left, colors[1]); \
  else printf("%%{Fblack}%s%%{F%s}", sep_l_left, colors[1]); \
  current_set_colors = colors;
