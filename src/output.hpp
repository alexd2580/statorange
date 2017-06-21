#ifndef __OUTPUT_MODULE_HEADER___
#define __OUTPUT_MODULE_HEADER___

#include <functional>
#include <iostream>
#include <string>

// The default number of displays. The output module will generate
// `num_output_displays`.
extern uint8_t num_output_displays;

namespace BarWriter
{
using Printer = std::function<void(std::ostream&)>;

enum class Coloring
{
    white_on_black,
    inactive,
    semiactive,
    active,
    warn,
    info,
    good,
    neutral
};

enum class Separator
{
    none, // The color abruptly changes.
    left, // A powerline separator pointing to the left.
    vertical, // A vertical separator.
    right // A powerline separator pointing to the right.
};

enum class Alignment
{
    left,
    center,
    right
};

// Icon glyphs from Terminusicons2
enum class Icon
{
    clock, // Clock
    cpu, // CPU
    mem, // MEM
    dl, // Download
    ul, // Upload
    vol, // Volume
    hd, // HD /
    home, // HD /home
    mail, // Mail
    chat, // IRC/Chat
    music, // Music
    prog, // Window
    contact, // Contact
    wsp, // Workspace
    wlan, // WIFI
    no_icon
};

std::pair<std::string, std::string> get_colors(Coloring);
Icon parse_icon(std::string const& s);
std::ostream& operator<<(std::ostream&, Icon);

void display(std::ostream&, uint8_t, Printer);
void align(std::ostream&, Alignment, Printer);
void button(std::ostream&, std::string const&, Printer);
void separator(std::ostream&, Separator);
void separator(std::ostream&, Separator, Coloring next);
void separator(
    std::ostream&, Separator, std::pair<std::string, std::string> const& next);
void separator(
    std::ostream&,
    Separator,
    std::string const& next_fg,
    std::string const& next_bg);

// void fixed_length_progress(
//     std::ostream& out,
//     uint8_t min_length,
//     float progress,
//     Separator left,
//     Separator right,
//     std::pair<std::string, std::string> const& colors_left,
//     std::pair<std::string, std::string> const& color_right,
//     std::string const& text);

std::string make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b);
template <typename T>
std::pair<std::string, std::string> section_colors(T value, T min, T max)
{
    if(value <= min || value >= max)
        return get_colors(value <= min ? Coloring::neutral : Coloring::warn);
    uint8_t byteV =
        (uint8_t)(((float)value - (float)min) * 256.0f / (float)(max - min));
    auto res = make_hex_color(255, byteV, (uint8_t)(127 - byteV / 2), 0);
    return {res, "#FFCCCCCC"};
}
}

// enum class WorkspaceGroup
// {
//   all,     // all workspaces
//   visible, // only directly visible ones
//   active,  // only the current ws
//   none     // none
// };
//
// WorkspaceGroup parse_workspace_group(std::string const& s);

#endif
