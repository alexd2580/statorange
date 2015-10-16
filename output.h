#ifndef __OUTPUT_MODULE_HEADER___
#define __OUTPUT_MODULE_HEADER___

#include<stdint.h>

#include"i3state.h"
#include"systemState.h"

void init_colors(void);

void echoPrimaryLemon(I3State* i3, SystemState* sys, Output* disp, uint8_t pos);
void echoSecondaryLemon(I3State* i3, SystemState* sys, Output* disp, uint8_t pos);

#endif
