#ifndef __LEMONBAR_WRITER_CLASS__
#define __LEMONBAR_WRITER_CLASS__

#include <iostream>
#include <map>
#include <string>

#include "util.hpp"

class Lemonbar final {
  public:
    // Icon glyphs from Terminusicons2.
    enum class Icon {
        no_icon,

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

        // Filling separators.
        right_fill,
        sep_left,
        sep_right,
        // Line separators.
        sep_l_vertical,
        sep_l_left,
        sep_l_right,
    };
    static Icon parse_icon(std::string const& s);
    static std::string const& icon(Icon);

    // Aligmnent on the top bar.
    enum class Alignment { left, center, right };

    // Used to divide sections.
    enum class Separator {
        none,     // The color abruptly changes.
        left,     // A powerline separator pointing to the left.
        vertical, // A vertical separator.
        right     // A powerline separator pointing to the right.
    };

    // Color schemes.
    enum class Coloring {
        // Default scheme.
        white_on_black,
        // Different activity states.
        inactive,
        semiactive,
        active,
        // Different quality states.
        good,
        neutral,
        info,
        warn,
        critical
    };

    using ColorPair = std::pair<std::string, std::string>;
    static std::map<Coloring, ColorPair const> const color_pairs;

    Lemonbar(std::ostream&);
    ~Lemonbar() = default;

    std::ostream& operator()() const;

    void display_begin(uint8_t);
    void display_end() const;

    void align_begin(Alignment);
    void align_end() const;

    void button_begin(std::string const& command);
    void button_end() const;

    template <typename T>
    static ColorPair section_colors(T value, T min, T max) {
        if(value <= min || value >= max) {
            return color_pairs.at(value <= min ? Coloring::neutral : Coloring::warn);
        }
        uint8_t byte_v = (uint8_t)(((float)value - (float)min) * 256.0f / (float)(max - min));
        auto res = make_hex_color(255, byte_v, (uint8_t)(127 - byte_v / 2), 0);
        return {res, "#FFCCCCCC"};
    }

    void separator(Separator);
    void separator(Separator, Coloring);
    void separator(Separator, std::string const& next_bg, std::string const& next_fg);

  private:
    std::ostream& out;

    uint8_t current_display;
    char current_alignment;

    std::string current_bg = "#FF000000";
    std::string current_fg = "#FFFFFFFF";

    static std::map<Icon, std::string> icons;
};

std::ostream& operator<<(std::ostream& out, Lemonbar::Icon i);

#endif
