#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "Lemonbar.hpp"

// The icon identifiers are named the same in `Icon` as they are in the input string.
// #define PARSE_CASE(enum_id, constr_id) \
//     if(s == #constr_id) \
//         return enum_id::constr_id;

// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wunused-variable"

//#pragma clang diagnostic pop

std::map<Lemonbar::Coloring, Lemonbar::ColorPair const> const Lemonbar::color_pairs{
    {Lemonbar::Coloring::white_on_black, {"#FF000000", "#FFFFFFFF"}},
    {Lemonbar::Coloring::inactive, {"#FF2A2A2A", "#FF707070"}},
    {Lemonbar::Coloring::semiactive, {"#FF454545", "#FFCCCCCC"}},
    {Lemonbar::Coloring::active, {"#FF1010D0", "#FFCCCCCC"}},
    {Lemonbar::Coloring::urgent, {"#FFD01010", "#FFCCCCCC"}},
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
void Lemonbar::button_end() const {
    out << "%{A}";
}

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
    auto const& separator =
        sep == Separator::left
            ? icon_sep_l_left
            : sep == Separator::vertical ? "vertical sep" : sep == Separator::right ? icon_sep_l_right : "";
    if(next_bg == current_bg) {
        out << "%{F#FF000000}" << separator;
    } else {
        switch(sep) {
        case Separator::none:
            out << "%{B" << next_bg << "}";
            break;
        case Separator::left:
            out << "%{F" << next_bg << "}" << icon_sep_left << "%{R}";
            break;
        case Separator::vertical:
            out << "%{F" << next_bg << "}" << "icon_right_fill" << "%{R}";
            break;
        case Separator::right:
            out << "%{R}%{B" << next_bg << "}" << icon_sep_right;
            break;
        }
    }
    out << "%{F" << next_fg << "}";

    current_fg = next_fg;
    current_bg = next_bg;
}
