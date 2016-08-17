#include <cstring>
#include <map>
#include <string>

#include "StateItem.hpp"
#include "output.hpp"

using namespace std;

/******************************************************************************/
/*****************************     ICONS     **********************************/

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

static map<Icon, string> icons{
    {Icon::clock, "Õ"},   // Clock icon
    {Icon::cpu, "Ï"},     // CPU icon
    {Icon::mem, "Þ"},     // MEM icon
    {Icon::dl, "Ð"},      // Download icon
    {Icon::ul, "Ñ"},      // Upload icon
    {Icon::vol, "Ô"},     // Volume icon
    {Icon::hd, "À"},      // HD / icon
    {Icon::home, "Æ"},    // HD /home icon
    {Icon::mail, "Ó"},    // Mail icon
    {Icon::chat, "Ò"},    // IRC/Chat icon
    {Icon::music, "Î"},   // Music icon
    {Icon::prog, "Â"},    // Window icon
    {Icon::contact, "Á"}, // Contact icon
    {Icon::wsp, "É"},     // Workspace icon
    {Icon::wlan, "Ø"},    // WIFI icon
    {Icon::no_icon, ""},
};

#define PARSE_CASE(enum_id, constr_id)                                         \
  if(s == #constr_id)                                                          \
    return enum_id::constr_id;

Icon parse_icon(std::string& s)
{
  PARSE_CASE(Icon, clock)
  PARSE_CASE(Icon, cpu)
  PARSE_CASE(Icon, mem)
  PARSE_CASE(Icon, dl)
  PARSE_CASE(Icon, ul)
  PARSE_CASE(Icon, vol)
  PARSE_CASE(Icon, hd)
  PARSE_CASE(Icon, home)
  PARSE_CASE(Icon, mail)
  PARSE_CASE(Icon, chat)
  PARSE_CASE(Icon, music)
  PARSE_CASE(Icon, prog)
  PARSE_CASE(Icon, contact)
  PARSE_CASE(Icon, wsp)
  PARSE_CASE(Icon, wlan)
  return Icon::no_icon;
}

std::ostream& operator<<(std::ostream& o, Icon i)
{
  return o << "%{T3}" << icons[i] << "%{T1}";
}

/******************************************************************************/
/******************************* WORKSPACES ***********************************/

WorkspaceGroup parse_workspace_group(std::string& s)
{
  PARSE_CASE(WorkspaceGroup, all)
  PARSE_CASE(WorkspaceGroup, visible)
  PARSE_CASE(WorkspaceGroup, active)
  return WorkspaceGroup::none;
}

/******************************************************************************/
/*****************************     COLORS   ***********************************/

static string const hex_color = "0123456789ABCDEF";

void make_hex(std::string::iterator dst, uint8_t a)
{
  *dst = hex_color[a / 16];
  *(dst + 1) = hex_color[a % 16];
}

string make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
  string res{"123456789"};
  auto it = res.begin();
  *it = '#';
  make_hex(it + 1, a);
  make_hex(it + 3, r);
  make_hex(it + 5, g);
  make_hex(it + 7, b);
  return res;
}

enum class ColorComponent
{
  white,
  dwhite,
  lgrey,
  grey,
  dgrey,
  red,
  green,
  dgreen,
  blue,
  yellow,
  blind,
  crimson,
  black,
  custom_bg,
  custom_fg,
  current_bg,
  current_fg
};

static map<ColorComponent, string> color_components{
    {ColorComponent::white, "#FFFFFFFF"},
    {ColorComponent::dwhite, "#FFCCCCCC"},
    {ColorComponent::lgrey, "#FF707070"},
    {ColorComponent::grey, "#FF454545"},
    {ColorComponent::dgrey, "#FF2A2A2A"},
    {ColorComponent::red, "#FFFF0000"},
    {ColorComponent::blue, "#FF1010D0"},
    {ColorComponent::green, "#FF10D010"},
    {ColorComponent::dgreen, "#FF008000"},
    {ColorComponent::yellow, "#FFCDCD00"},
    {ColorComponent::blind, "#FF8B814C"},
    {ColorComponent::crimson, "#FFDC143C"},
    {ColorComponent::black, "#FF000000"},
    {ColorComponent::custom_bg, "#00000000"},
    {ColorComponent::custom_fg, "#00000000"},
    {ColorComponent::current_bg, "#00000000"},
    {ColorComponent::current_fg, "#00000000"},
};

/******************************************************************************/

static map<Color, pair<ColorComponent, ColorComponent> const> color_pairs{
    {Color::white_on_black, {ColorComponent::black, ColorComponent::white}},
    {Color::inactive, {ColorComponent::dgrey, ColorComponent::lgrey}},
    {Color::semiactive, {ColorComponent::grey, ColorComponent::dwhite}},
    {Color::active, {ColorComponent::blue, ColorComponent::dwhite}},
    {Color::warn, {ColorComponent::red, ColorComponent::dwhite}},
    {Color::info, {ColorComponent::yellow, ColorComponent::black}},
    {Color::good, {ColorComponent::green, ColorComponent::black}},
    {Color::neutral, {ColorComponent::dgreen, ColorComponent::dwhite}},
    {Color::current, {ColorComponent::current_bg, ColorComponent::current_fg}},
    {Color::custom, {ColorComponent::custom_bg, ColorComponent::custom_fg}},
};
/**************************     SEPARATORS ********************************/

// Char glyps for powerline fonts
static string const sep_left{""};    // Powerline separator left
static string const sep_right{""};   // Powerline separator right
static string const sep_l_left{""};  // Powerline light separator left
static string const sep_l_right{""}; // Powerline light sepatator right

#pragma clang diagnostic pop

template <typename T> class TD;

void separate(Direction d, ostream& o) { separate(d, Color::current, o); }

void separate(Direction d, Color color, ostream& o)
{
  auto const& bg_cur = color_components[ColorComponent::current_bg];
  auto const& color_pair = color_pairs[color];
  auto const& bg_col = color_components[color_pair.first];
  auto const& fg_col = color_components[color_pair.second];

  if(d == Direction::left)
  {
    if(bg_col != bg_cur)
      o << "%{F" << bg_col << '}' << sep_left << "%{R}%{F" << fg_col << '}';
    else
      o << "%{F#FF000000}" << sep_l_left << "%{F" << fg_col << '}';
  }
  else
  {
    if(bg_col != bg_cur)
      o << "%{R}%{B" << bg_col << '}' << sep_right << "%{F" << fg_col << '}';
    else
      o << "%{F#FF000000}" << sep_l_right << "%{F" << fg_col << '}';
  }

  color_components[ColorComponent::current_bg] = bg_col;
  color_components[ColorComponent::current_fg] = fg_col;
}

void dynamic_section(float value, float min, float max, ostream& o)
{
  if(value <= min)
    separate(Direction::left, Color::neutral, o);
  else if(value >= max)
    separate(Direction::left, Color::warn, o);
  else
  {
    uint8_t byteV = (uint8_t)((value - min) * 256.0f / (max - min));
    string res = make_hex_color(255, byteV, (uint8_t)(127 - byteV / 2), 0);

    color_components[ColorComponent::custom_bg] = res;
    auto const& fg_color = color_components[ColorComponent::dwhite];
    color_components[ColorComponent::custom_fg] = fg_color;
    separate(Direction::left, Color::custom, o);
  }
}

/******************************************************************************/
/***************************     BUTTONS **********************************/

void startButton(string cmd, ostream& o)
{
  o << "%{A:";
  for(auto i = cmd.begin(); i != cmd.end(); i++)
    o << (*i == ':' ? "\\" : "") << *i;
  o << ":}";
}

void stopButton(ostream& o) { o << "%{A}"; }

/******************************************************************************/
/****************************     OUTPUT **********************************/

/**
 * Prints the buttons for switching workspaces
 * align them to the left side
 */
void echo_workspace_buttons(I3State const& i3,
                            WorkspaceGroup show_names,
                            Output const& disp)
{
  for(auto& workspace_pair : disp.workspaces)
  {
    auto& workspace = *workspace_pair.second;

    Color color = Color::inactive;
    if(workspace.urgent)
      color = Color::warn;
    else if(workspace.focused)
      color = Color::active;
    else if(workspace.visible)
      color = Color::semiactive;

    startButton("i3-msg workspace " + workspace.name);
    separate(Direction::right, color);
    cout << ' ' << workspace.name << ' ';

    /*bool show_name =
        show_names == WorkspaceGroup::all ||
        (show_names == WorkspaceGroup::visible && workspace.visible) ||
        (show_names == WorkspaceGroup::active && workspace.focused);


        if(show_name && workspace.focusedApp.size() != 0)
        {
          separate(Direction::right, color);
          cout << ' ' << workspace.focusedApp << ' ';
      }*/
    separate(Direction::right, Color::white_on_black);
    stopButton();
  }
}

/**
 * Prints the input to lemonbar
 * TODO specify main monitor in config -> fix main, not this method
 */
void echo_lemon(I3State const& i3, WorkspaceGroup show_names)
{
  for(auto& output_pair : i3.outputs)
  {
    auto& output = output_pair.second;
    cout << "%{S" << (int)output.position << "}";
    cout << "%{l}";
    echo_workspace_buttons(i3, show_names, output);
    if(i3.mode.compare("default") != 0)
    {
      cout << ' ';
      separate(Direction::left, Color::info);
      cout << ' ' << i3.mode << ' ';
      separate(Direction::right, Color::white_on_black);
    }
    cout << "%{l}";
    cout << "%{r}";

    StateItem::printState();

    cout << "%{r}";
    cout << "%{S" << (int)output.position << "}";
  }

  cout << endl;
}

/* ************************************************************************** */
