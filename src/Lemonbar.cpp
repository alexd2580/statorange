#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "Lemonbar.hpp"

// The icon identifiers are named the same in `Icon` as they are in the input string.
#define PARSE_CASE(enum_id, constr_id)                                                                                 \
    if(s == #constr_id)                                                                                                \
        return enum_id::constr_id;

Lemonbar::Icon Lemonbar::parse_icon(std::string const& s) {
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

std::string const& Lemonbar::icon(Icon i) { return icons[i]; }

// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wunused-variable"

//#pragma clang diagnostic pop

std::map<Lemonbar::Coloring, Lemonbar::ColorPair const> const Lemonbar::color_pairs{
    {Lemonbar::Coloring::white_on_black, {"#FF000000", "#FFFFFFFF"}},
    {Lemonbar::Coloring::inactive, {"#FF2A2A2A", "#FF707070"}},
    {Lemonbar::Coloring::semiactive, {"#FF454545", "#FFCCCCCC"}},
    {Lemonbar::Coloring::active, {"#FF1010D0", "#FFCCCCCC"}},
    {Lemonbar::Coloring::good, {"#FF10D010", "#FF000000"}},
    {Lemonbar::Coloring::neutral, {"#FF008000", "#FFCCCCCC"}},
    {Lemonbar::Coloring::info, {"#FFCDCD00", "#FF000000"}},
    {Lemonbar::Coloring::warn, {"#FFD01010", "#FFCCCCCC"}},
    {Lemonbar::Coloring::critical, {"#FFFF0000", "#FFFFFFFF"}},
};

Lemonbar::Lemonbar(std::ostream& ostr) : out(ostr) {}

std::ostream& Lemonbar::operator()() const { return out; }

void Lemonbar::display_begin(uint8_t num_display) {
    current_display = num_display;
    out << "%{S" << static_cast<int>(current_display) << "}";
}
void Lemonbar::display_end() const { out << "%{S" << static_cast<int>(current_display) << "}"; }

void Lemonbar::align_begin(Alignment a) {
    current_alignment = a == Alignment::center ? 'c' : a == Alignment::left ? 'l' : 'r';
    out << "%{" << current_alignment << "}";
}
void Lemonbar::align_end() const { out << "%{" << current_alignment << "}"; }

void Lemonbar::button_begin(std::string const& command) {
    out << "%{A:";
    for(char const& c : command) {
        out << (c == ':' ? "\\:" : "") << c;
    }
    out << ":}";
}
void Lemonbar::button_end() const { out << "%{A}"; }

void Lemonbar::make_hex(std::string::iterator dst, uint8_t a) {
    static std::string const hex_chars = "0123456789ABCDEF";
    *dst = hex_chars[a / 16];
    *(dst + 1) = hex_chars[a % 16];
}

std::string Lemonbar::make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    std::string res{"123456789"};
    auto it = res.begin();
    *it = '#';
    make_hex(it + 1, a);
    make_hex(it + 3, r);
    make_hex(it + 5, g);
    make_hex(it + 7, b);
    return res;
}

void Lemonbar::separator(Separator sep) { separator(sep, current_bg, current_fg); }
void Lemonbar::separator(Separator sep, Coloring next) {
    auto const& color_pair = color_pairs.at(next);
    separator(sep, color_pair.first, color_pair.second);
}
void Lemonbar::separator(Separator sep, std::string const& next_bg, std::string const& next_fg) {
    if(next_bg == current_bg) {
        out << "%{F#FF000000}"
            << (sep == Separator::left
                    ? Icon::sep_l_left
                    : sep == Separator::vertical ? Icon::sep_l_vertical
                                                 : sep == Separator::right ? Icon::sep_l_right : Icon::no_icon);
    } else {
        switch(sep) {
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

// Char glyps for powerline fonts
std::map<Lemonbar::Icon, std::string> Lemonbar::icons{
    {Lemonbar::Icon::no_icon, ""},

    {Lemonbar::Icon::clock, "Õ"},   // Clock icon
    {Lemonbar::Icon::cpu, "Ï"},     // CPU icon
    {Lemonbar::Icon::mem, "Þ"},     // MEM icon
    {Lemonbar::Icon::dl, "Ð"},      // Download icon
    {Lemonbar::Icon::ul, "Ñ"},      // Upload icon
    {Lemonbar::Icon::vol, "Ô"},     // Volume icon
    {Lemonbar::Icon::hd, "À"},      // HD / icon
    {Lemonbar::Icon::home, "Æ"},    // HD /home icon
    {Lemonbar::Icon::mail, "Ó"},    // Mail icon
    {Lemonbar::Icon::chat, "Ò"},    // IRC/Chat icon
    {Lemonbar::Icon::music, "Î"},   // Music icon
    {Lemonbar::Icon::prog, "Â"},    // Window icon
    {Lemonbar::Icon::contact, "Á"}, // Contact icon
    {Lemonbar::Icon::wsp, "É"},     // Workspace icon
    {Lemonbar::Icon::wlan, "Ø"},    // WIFI icon

    // Private use.
    {Lemonbar::Icon::right_fill, "▐"},     // Right block half
    {Lemonbar::Icon::sep_left, "Ü"},       // Powerline separator left
    {Lemonbar::Icon::sep_right, "Ú"},      // Powerline separator right
    {Lemonbar::Icon::sep_l_vertical, "│"}, // Vertical bar
    {Lemonbar::Icon::sep_l_left, "Ý"},     // Powerline light separator left
    {Lemonbar::Icon::sep_l_right, "Ú"},    // Powerline light sepatator right
};

std::ostream& operator<<(std::ostream& out, Lemonbar::Icon i) { return out << "%{T2}" << Lemonbar::icon(i) << "%{T1}"; }
