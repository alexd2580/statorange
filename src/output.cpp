#include <cmath>
#include <cstring>
#include <map>
#include <string>

#include "StateItem.hpp"
#include "output.hpp"

using namespace std;

uint8_t num_output_displays = 3;

// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wunused-variable"

namespace BarWriter
{
enum class Color
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
    black
};

static string const hex_chars = "0123456789ABCDEF";

static map<Color, string const> colors{{Color::white, "#FFFFFFFF"},
                                       {Color::dwhite, "#FFCCCCCC"},
                                       {Color::lgrey, "#FF707070"},
                                       {Color::grey, "#FF454545"},
                                       {Color::dgrey, "#FF2A2A2A"},
                                       {Color::red, "#FFFF0000"},
                                       {Color::blue, "#FF1010D0"},
                                       {Color::green, "#FF10D010"},
                                       {Color::dgreen, "#FF008000"},
                                       {Color::yellow, "#FFCDCD00"},
                                       {Color::blind, "#FF8B814C"},
                                       {Color::crimson, "#FFDC143C"},
                                       {Color::black, "#FF000000"}};

static map<Coloring, pair<Color, Color> const> color_pairs{
    {Coloring::white_on_black, {Color::black, Color::white}},
    {Coloring::inactive, {Color::dgrey, Color::lgrey}},
    {Coloring::semiactive, {Color::grey, Color::dwhite}},
    {Coloring::active, {Color::blue, Color::dwhite}},
    {Coloring::warn, {Color::red, Color::dwhite}},
    {Coloring::info, {Color::yellow, Color::black}},
    {Coloring::good, {Color::green, Color::black}},
    {Coloring::neutral, {Color::dgreen, Color::dwhite}},
};

// Char glyps for powerline fonts

static map<Icon, string> icons{
    {Icon::no_icon, ""},

    {Icon::clock, "Õ"}, // Clock icon
    {Icon::cpu, "Ï"}, // CPU icon
    {Icon::mem, "Þ"}, // MEM icon
    {Icon::dl, "Ð"}, // Download icon
    {Icon::ul, "Ñ"}, // Upload icon
    {Icon::vol, "Ô"}, // Volume icon
    {Icon::hd, "À"}, // HD / icon
    {Icon::home, "Æ"}, // HD /home icon
    {Icon::mail, "Ó"}, // Mail icon
    {Icon::chat, "Ò"}, // IRC/Chat icon
    {Icon::music, "Î"}, // Music icon
    {Icon::prog, "Â"}, // Window icon
    {Icon::contact, "Á"}, // Contact icon
    {Icon::wsp, "É"}, // Workspace icon
    {Icon::wlan, "Ø"}, // WIFI icon

    // Private use.
    {Icon::right_fill, "▐"}, // Right block half
    {Icon::sep_left, "Ü"}, // Powerline separator left
    {Icon::sep_right, "Ú"}, // Powerline separator right
    {Icon::sep_l_vertical, "│"}, // Vertical bar
    {Icon::sep_l_left, "Ý"}, // Powerline light separator left
    {Icon::sep_l_right, "Ú"}, // Powerline light sepatator right
};

//#pragma clang diagnostic pop

#define PARSE_CASE(enum_id, constr_id) \
    if(s == #constr_id)                \
        return enum_id::constr_id;

Icon parse_icon(string const& s)
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

pair<string, string> get_colors(Coloring c)
{
    auto const& color_pair = color_pairs[c];
    auto const& next_bg = colors[color_pair.first];
    auto const& next_fg = colors[color_pair.second];
    return {next_bg, next_fg};
}

ostream& operator<<(ostream& o, Icon i)
{
    return o << "%{T2}" << icons[i] << "%{T1}";
}

void make_hex(string::iterator dst, uint8_t a)
{
    *dst = hex_chars[a / 16];
    *(dst + 1) = hex_chars[a % 16];
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

void display(ostream& out, uint8_t display, Printer p)
{
    out << "%{S" << (int)display << "}";
    p(out);
    out << "%{S" << (int)display << "}";
}

void align(ostream& out, Alignment a, Printer p)
{
    char alignment =
        a == Alignment::center ? 'c' : a == Alignment::left ? 'l' : 'r';
    out << "%{" << alignment << "}";
    p(out);
    out << "%{" << alignment << "}";
}

void button(ostream& out, string const& cmd, Printer p)
{
    out << "%{A:";
    for(auto i = cmd.begin(); i != cmd.end(); i++)
        out << (*i == ':' ? "\\" : "") << *i;
    out << ":}";
    p(out);
    out << "%{A}";
}

static string current_bg = colors[Color::black];
static string current_fg = colors[Color::white];

void separator(ostream& out, Separator d)
{
    separator(out, d, current_fg, current_bg);
}

void separator(ostream& out, Separator d, Coloring next)
{
    auto const& color_pair = color_pairs[next];
    auto const& next_bg = colors[color_pair.first];
    auto const& next_fg = colors[color_pair.second];
    separator(out, d, next_fg, next_bg);
}

void separator(
    ostream& out, Separator d, pair<string, string> const& color_pair)
{
    separator(out, d, color_pair.second, color_pair.first);
}

void separator(
    ostream& out, Separator d, string const& next_fg, string const& next_bg)
{
    if(next_bg == current_bg)
    {
        auto const& separator =
            d == Separator::left
                ? Icon::sep_l_left
                : d == Separator::vertical
                      ? Icon::sep_l_vertical
                      : d == Separator::right ? Icon::sep_l_right : Icon::no_icon;
        out << "%{F#FF000000}" << separator;
    }
    else
    {
        switch(d)
        {
        case Separator::none:
            out << "%{B" << next_bg << "}";
            break;
        case Separator::left:
            out << "%{F" << next_bg << "}" << Icon::sep_left << "%{R}";
            break;
        case Separator::vertical:
            out << "%{F" << next_bg << "}" << Icon::right_fill << "%{R}";
            break;
        case Separator::right:
            out << "%{R}%{B" << next_bg << "}" << Icon::sep_right;
            break;
        }
    }
    out << "%{F" << next_fg << "}";

    current_fg = next_fg;
    current_bg = next_bg;
}

// void fixed_length_progress(
//     ostream& out,
//     uint8_t min_text_length,
//     float progress,
//     Separator left,
//     Separator right,
//     pair<string, string> const& colors_left,
//     pair<string, string> const& color_right,
//     string const& text)
// {
//     uint8_t total_length = 2 * (min_text_length + 2) + 1;
//     bool has_left_sep = left != Separator::none;
//     bool has_right_sep = right != Separator::none;
//     total_length += has_left_sep ? 1 : 0;
//     total_length += has_right_sep ? 1 : 0;
//
//     uint8_t left_length = (uint8_t)floor((float)total_length * progress);
//     uint8_t right_length = total_length - left_length;
//
//     bool text_is_left = progress > 0.5f;
//
//     // Left Part.
//     if(round(progress) == 0.0f)
//     {
//         separator(out, left, colors_right);
//     }
//     else
//     {
//         // If the progress is zero then the first separator is empty-colored.
//         separator(out, left, colors_left);
//         uint8_t left_to_fill = left_length - (no_left_sep ? 0 : 1);
//         if(text_is_left)
//         {
//             out << " " << text << " ";
//             left_to_fill -= text.length() + 2;
//         }
//         out << string(left_to_fill, ' ');
//     }
//
//     // Right part.
//     if(round(progress) == 1.0f)
//     {
//     }
//     else
//     {
//         uint8_t right_to_fill = right_length - (no_right_sep ? 0 : 1);
//     }
//
//     // Closing separator.
//     separator(out, right, Coloring::white_on_black);
// }

/**
 * Prints the buttons for switching workspaces.
 * Align them to the left side.
 */
// void echo_workspace_buttons(I3State& i3,
//                             WorkspaceGroup show_names,
//                             Output const& disp,
//                             bool focused)
// {
//     for(auto const& workspace_pair : disp.workspaces)
//     {
//         auto const workspace_ptr = workspace_pair.second;
//         auto const& workspace = *workspace_ptr;
//         bool visible = workspace_ptr.get() ==
//         disp.focused_workspace.get();
//
//         Coloring color = Coloring::inactive;
//         if(workspace.urgent)
//             color = Coloring::warn;
//         else if(focused && visible)
//             color = Coloring::active;
//         else if(visible)
//             color = Coloring::semiactive;
//
//         startButton("i3-msg workspace " + workspace.name);
//         separate(Separator::right, color);
//         cout << ' ' << workspace.name << ' ';
//
//         bool show_name =
//             show_names == WorkspaceGroup::all ||
//             (show_names == WorkspaceGroup::visible && visible) ||
//             (show_names == WorkspaceGroup::active && focused && visible);
//
//         if(show_name && workspace.focused_window_id != -1)
//         {
//             auto iter = i3.windows.find(workspace.focused_window_id);
//             if(iter != i3.windows.end())
//             {
//                 separate(Separator::right, color);
//                 cout << ' ' << iter->second.name << ' ';
//             }
//         }
//         separate(Separator::right, Coloring::white_on_black);
//         stopButton();
//     }
// }

// void echo_lemon(void)
// {
//     for(auto const& output_pair : i3.outputs)
//     {
//         auto& output = output_pair.second;
//         cout << "%{S" << (int)output->position << "}";
//         cout << "%{l}";
//         bool focused = i3.focused_output.get() == output.get();
//         echo_workspace_buttons(i3, show_names, *output, focused);
//         if(i3.mode.compare("default") != 0)
//         {
//             cout << ' ';
//             separate(Separator::left, Coloring::info);
//             cout << ' ' << i3.mode << ' ';
//             separate(Separator::right, Coloring::white_on_black);
//         }
//         cout << "%{l}";
//         cout << "%{r}";
//
//         StateItem::print_state();
//
//         cout << "%{r}";
//         cout << "%{S" << (int)output->position << "}";
//     }
//
//     cout << endl;
// }
}
// WorkspaceGroup parse_workspace_group(string const& s)
// {
//     PARSE_CASE(WorkspaceGroup, all)
//     PARSE_CASE(WorkspaceGroup, visible)
//     PARSE_CASE(WorkspaceGroup, active)
//     return WorkspaceGroup::none;
// }
