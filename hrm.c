// heart rate monitor code

//#include <string.h>

#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "adc0.h"
#include "resp.h"

#include "uart0.h"

#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define INPUT_LED    (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 0*4)))

// PortB masks
#define AIN11_MASK 32

// PortC masks
#define FREQ_IN_MASK 64

// PortD masks
#define INPUT_MASK 1
#define DATA_MASK 64

// PortF masks
#define BLUE_LED_MASK 4

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

bool pulse_active = false;
uint8_t timePassed = 0;
uint8_t index = 0;
int16_t v1 = 0;
int16_t v2 = 0;
uint32_t BPM = 0;
uint32_t sum = 0;
uint32_t avg = 0;
uint32_t pulse_time = 0;
uint32_t prevTime = 0;
uint32_t x[5] = {0, 0, 0, 0, 0};

char str[40];


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void enableTimer1()
{
    // Configure Timer 1 as the time base
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_TAILR_R = 10000000;                       // set load value to 20e6 for 2 Hz interrupt rate (.5 seconds)
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN0_R |= 1 << (INT_TIMER1A-16);             // turn-on interrupt 37 (TIMER1A)
}

void disableTimer1()
{
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off time base timer
    NVIC_DIS0_R |= 1 << (INT_TIMER1A-16);            // turn-off interrupt 37 (TIMER1A)
}

void enableTimer2()
{
    // Configure Timer 2 as the time base
    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER2_TAILR_R = 40000000;                       // set load value to 40e6 for 1 Hz interrupt rate (1 second)
    TIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    TIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN0_R |= 1 << (INT_TIMER2A-16);             // turn-on interrupt 39 (TIMER2A)
}

void disableTimer2()
{
    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off time base timer
    NVIC_DIS0_R |= 1 << (INT_TIMER2A-16);            // turn-off interrupt 39 (TIMER2A)
}

void enableWTimer1()
{
    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER1_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER1_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;
                                                     // configure for edge time mode, count up
    WTIMER1_CTL_R = TIMER_CTL_TAEVENT_POS;           // measure time from positive edge to positive edge
    WTIMER1_IMR_R = TIMER_IMR_CAEIM;                 // turn-on interrupts
    WTIMER1_TAV_R = 0;                               // zero counter for first period
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
    NVIC_EN3_R |= 1 << (INT_WTIMER1A-16-96);         // turn-on interrupt 112 (WTIMER1A)
}


// Frequency counter service publishing latest frequency measurements every second
void timer1Isr()
{
    if (pulse_active)
    {
        if (pulse_time == prevTime || BPM > 250 || BPM < 30)        // if the previous time is equal to current
            timePassed = timePassed + 1;                // increment the idle counter
        else
            timePassed = 0;                             // o/w, set the counter to 0
        if (timePassed == 6)                            // if 3s have passed, turn off the LED
        {
            INPUT_LED = 0;
            pulse_active = false;
            putsUart0("Finger removed\n");
            enablePortDISR();
            enableTimer2();
        }
        prevTime = pulse_time;                          // set the previous time to the current time
    }
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;              // clear interrupt flag*/
}

void timer2Isr()
{
    if (!pulse_active)
    {
        v1 = readAdc0Ss3();
        if (v1 > 3400)                      // dark scenario
        {
            INPUT_LED = 1;
            waitMicrosecond(500);
            v1 = readAdc0Ss3();
            waitMicrosecond(500);
            INPUT_LED = 0;
            waitMicrosecond(100);
            v2 = readAdc0Ss3();
            if (v2 > v1 && v2 - v1 > 0)
            {
                putsUart0("Finger detected, hold still...\n");
                pulse_active = true;
                INPUT_LED = 1;
                waitMicrosecond(5000000);
                putsUart0("BPM now being calculated\n");
            }
        }
        if (v1 < 3400) // bright scenario
        {
            INPUT_LED = 1;
            waitMicrosecond(300);
            v1 = readAdc0Ss3();
            waitMicrosecond(200);
            INPUT_LED = 0;
            waitMicrosecond(100);
            v2 = readAdc0Ss3();

            if (v2 > v1 && v2 - v1 > 50)
            {
                putsUart0("Finger detected, hold still...\n");
                pulse_active = true;
                INPUT_LED = 1;
                waitMicrosecond(5000000);
                putsUart0("BPM now being calculated\n");
            }
        }
    }

    TIMER2_ICR_R = TIMER_ICR_TATOCINT; // clear interrupt flag
    if (pulse_active)
    {
        disableTimer2();
        enableTimer1();
    }
}

// Period timer service publishing latest time measurements every positive edge
void wideTimer1Isr()
{
    pulse_time = WTIMER1_TAV_R;                  // read counter input
    WTIMER1_TAV_R = 0;                           // zero counter for next edge

    if (pulse_active)
    {
        BPM = 60000000 / (pulse_time / 40);
        if (BPM <= 250 && BPM >= 30 &&
           (BPM - x[(index + 1) % 5] > 5 || x[(index + 2) % 5] - BPM > 5) &&
           (BPM - x[(index + 2) % 5] > 5 || x[(index + 2) % 5] - BPM > 5) &&
           (BPM - x[(index + 3) % 5] > 5 || x[(index + 3) % 5] - BPM > 5) &&
           (BPM - x[(index + 4) % 5] > 5 || x[(index + 4) % 5] - BPM > 5))
        {
            sum -= x[index];
            sum += BPM;
            x[index] = BPM;
            index = (index + 1) % 5;
            avg = sum / 5;
        }
    }
    WTIMER1_ICR_R = TIMER_ICR_CAECINT;           // clear interrupt flag
}

// Initialize Hardware
void initFingHW()
{
    // Enable clocks
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1 | SYSCTL_RCGCTIMER_R2 | SYSCTL_RCGCTIMER_R3;
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R2 | SYSCTL_RCGCGPIO_R3 | SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R |= BLUE_LED_MASK;                  // bits 1 and 2 are outputs, other pins are inputs
    GPIO_PORTD_DIR_R |= INPUT_MASK;
    GPIO_PORTF_DEN_R |= BLUE_LED_MASK;
    GPIO_PORTD_DEN_R |= INPUT_MASK;                     // enable LEDs and pushbuttons

    // Configure SIGNAL_IN for frequency and time measurements
    GPIO_PORTC_AFSEL_R |= FREQ_IN_MASK;                 // select alternative functions for SIGNAL_IN pin
    GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC6_M;              // map alt fns to SIGNAL_IN
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC6_WT1CCP0;
    GPIO_PORTC_DEN_R |= FREQ_IN_MASK;                   // enable bit 6 for digital input

    // Configure AIN11 as an analog input
    GPIO_PORTB_AFSEL_R |= AIN11_MASK;                   // select alternative functions for AN11 (PB5)
    GPIO_PORTB_DEN_R &= ~AIN11_MASK;                    // turn off digital operation on pin PB5
    GPIO_PORTB_AMSEL_R |= AIN11_MASK;                   // turn on analog operation on pin PB5
}
