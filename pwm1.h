// RGB LED Library - Modified to only activate red LED
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD Interface
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red Backlight LED:
//   M1PWM5 (PF1) drives an NPN transistor that powers the red LED
// Green Backlight LED:
//   M1PWM7 (PF3) drives an NPN transistor that powers the green LED
// Blue Backlight LED:
//   M1PWM6 (PF2) drives an NPN transistor that powers the blue LED

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef PWM1_H_
#define PWM1_H_

#include <stdint.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initPWM();
void setPWMCMP(uint16_t load);

#endif
