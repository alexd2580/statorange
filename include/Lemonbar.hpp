#ifndef LEMONBAR_HPP
#define LEMONBAR_HPP

#include <iostream>
#include <map>
#include <string>

class Lemonbar final {
  public:
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
        urgent,
        // Different quality states.
        good,
        neutral,
        info,
        warn,
        critical
    };

    using ColorPair = std::pair<std::string, std::string>;
    static std::map<Coloring, ColorPair const> const color_pairs;

    explicit Lemonbar(std::ostream&);

    std::ostream& operator()() const;

    void display_begin(uint8_t);
    void display_end() const;

    void align_begin(Alignment);
    void align_end() const;

    void button_begin(std::string const& command);
    void button_end() const;

    static void make_hex(std::string::iterator dst, uint8_t a);
    static std::string make_hex_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b);

    template <typename T>
    static float inv_interpolate(T value, T min, T max) {
        return static_cast<float>(value - min) / static_cast<float>(max - min);
    }

    template <typename T>
    static ColorPair section_colors(T value, T min, T max) {
        if(value <= min) {
            return color_pairs.at(Coloring::neutral);
        }
        if(value >= max) {
            return color_pairs.at(Coloring::warn);
        }
        auto byte_v = static_cast<uint8_t>(inv_interpolate(value, min, max) * 256.f);
        auto res = make_hex_color(255, byte_v, static_cast<uint8_t>(127 - byte_v / 2), 0);
        return {res, "#FFCCCCCC"};
    }

    void separator(Separator);
    void separator(Separator, Coloring);
    void separator(Separator, std::string const& next_bg, std::string const& next_fg);


    // Fill Powerline separators.
    std::string const icon_sep_left = "\ue0b2";
    std::string const icon_sep_right = "\ue0b0";

    // Line Powerline separators.
    std::string const icon_sep_l_left = "\ue0b3";
    std::string const icon_sep_l_right = "\ue0b1";

    // Powerline separator right
    // Right block half.
    // std::string const icon_right_fill = "▐";
    // std::string const icon_sep_l_vertical = "│";


  private:
    std::ostream& out;

    uint8_t current_display;
    char current_alignment;

    std::string current_bg = "#FF000000";
    std::string current_fg = "#FFFFFFFF";
};

#endif
