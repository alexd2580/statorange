#ifndef __OUTPUT_MODULE_HEADER___
#define __OUTPUT_MODULE_HEADER___

/******************************************************************************/
/*****************************     ICONS     **********************************/

#include<iostream>

static inline void print_icon(char const* const icon, std::ostream& o = std::cout)
{
  o << "%{T3}" << icon << "%{T1}";
}

// Icon glyphs from Terminusicons2
extern char const* const icon_clock;          // Clock icon
extern char const* const icon_cpu;            // CPU icon
extern char const* const icon_mem;            // MEM icon
extern char const* const icon_dl;             // Download icon
extern char const* const icon_ul;             // Upload icon
extern char const* const icon_vol;            // Volume icon
extern char const* const icon_hd;             // HD / icon
extern char const* const icon_home;           // HD /home icon
extern char const* const icon_mail;           // Mail icon
extern char const* const icon_chat;           // IRC/Chat icon
extern char const* const icon_music;          // Music icon
extern char const* const icon_prog;           // Window icon
extern char const* const icon_contact;        // Contact icon
extern char const* const icon_wsp;            // Workspace icon
extern char const* const icon_wlan;           // WIFI icon 

/******************************************************************************/
/*****************************     COLORS     *********************************/

extern char const* white_on_black[2]; //{ color_black, color_white };
extern char const* inactive_colors[2]; //{ color_dgrey, color_lgrey };
extern char const* semiactive_colors[2]; //{ color_grey, color_dwhite };
extern char const* active_colors[2]; //{ color_blue, color_dwhite };
extern char const* warn_colors[2]; //{ color_red, color_dwhite };
extern char const* info_colors[2]; //{ color_yellow, color_black };
extern char const* good_colors[2]; //{ color_green, color_black };
extern char const* neutral_colors[2]; //{ color_dgreen, color_black };

enum Direction
{
  Left, Right
};

void separate(Direction d, char const** colors, std::ostream& = std::cout);
void dynamic_section(float value, float min, float max, std::ostream& = std::cout);

#include<string>

void startButton(std::string cmd, std::ostream& = std::cout);
void stopButton(std::ostream& = std::cout);

#include<cstdint>
#include"i3state.hpp"

void init_colors(void);

void echoPrimaryLemon(I3State& i3, uint8_t disp);
void echoSecondaryLemon(I3State& i3, uint8_t disp);

#endif
