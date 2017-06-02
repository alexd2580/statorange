#ifndef __OUTPUT_MODULE_HEADER___
#define __OUTPUT_MODULE_HEADER___

/******************************************************************************/
/*****************************     ICONS     **********************************/

#include <iostream>

// Icon glyphs from Terminusicons2
enum class Icon
{
  clock,   // Clock
  cpu,     // CPU
  mem,     // MEM
  dl,      // Download
  ul,      // Upload
  vol,     // Volume
  hd,      // HD /
  home,    // HD /home
  mail,    // Mail
  chat,    // IRC/Chat
  music,   // Music
  prog,    // Window
  contact, // Contact
  wsp,     // Workspace
  wlan,    // WIFI
  no_icon
};

Icon parse_icon(std::string const& s);
std::ostream& operator<<(std::ostream&, Icon);

/******************************************************************************/
/*****************************     COLORS     *********************************/

enum class Color
{
  white_on_black,
  inactive,
  semiactive,
  active,
  warn,
  info,
  good,
  neutral,
  current,
  custom
};

enum class Direction
{
  left,
  right
};

/** Matches al 4 cases **/
void separate(Direction d, std::ostream& o);
void separate(Direction d,
              Color colors = Color::current,
              std::ostream& = std::cout);

void dynamic_section(float value,
                     float min,
                     float max,
                     std::ostream& = std::cout);

#include <string>

void startButton(std::string cmd, std::ostream& = std::cout);
void stopButton(std::ostream& = std::cout);

#include "i3state.hpp"

enum class WorkspaceGroup
{
  all,     // all workspaces
  visible, // only directly visible ones
  active,  // only the current ws
  none     // none
};

WorkspaceGroup parse_workspace_group(std::string const& s);

void echo_lemon(I3State& i3, WorkspaceGroup show_window_names);

#endif
