// RGB LED Library - Modified to only activate speaker, and larger Load value
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

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <pwm1.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"

// PortF masks
#define SPEAKER_MASK 2

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize RGB
void initPWM()
{
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
    _delay_cycles(3);

    // Configure three LEDs
    GPIO_PORTD_DEN_R |= SPEAKER_MASK;
    GPIO_PORTD_AFSEL_R |= SPEAKER_MASK;
    GPIO_PORTD_PCTL_R &= ~GPIO_PCTL_PD1_M;
    GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD1_M1PWM1;

    // Configure PWM module 1 to drive RGB LED
    // RED   on M1PWM5 (PF1), M1PWM2b
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1;                // reset PWM1 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state
    PWM1_0_CTL_R = 0;                                // turn-off PWM1 generator 0 (drives outs 4 and 5)
    PWM1_0_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;
                                                     // output 5 on PWM1, gen 0b, cmpb

    PWM1_0_LOAD_R = 16384;                            // set frequency to 40 MHz sys clock / (2 * 1024) = 19.53125 kHz
													 // freq = sysclk / (2 * load)

    PWM1_0_CMPB_R = 0;                               // red off (0=always low, 1023=always high)

    PWM1_0_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM1 generator 2
    PWM1_ENABLE_R = PWM_ENABLE_PWM1EN;
                                                     // enable outputs
}

void setPWMCMP(uint16_t load)
{
    PWM1_0_CMPB_R = load;
}
