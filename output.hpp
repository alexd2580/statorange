#ifndef __OUTPUT_MODULE_HEADER___
#define __OUTPUT_MODULE_HEADER___

#include<cstdint>

#include"i3state.hpp"
#include"systemState.hpp"

void init_colors(void);

void echoPrimaryLemon(I3State& i3, SystemState& sys, Output& disp, uint8_t pos);
void echoSecondaryLemon(I3State& i3, SystemState& sys, Output& disp, uint8_t pos);

#endif
