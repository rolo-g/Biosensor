// respiration code


#include <inttypes.h>
#include <stdio.h>
#include <string.h>


#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"

#include "uart0.h"

// Bitband aliases
#define DATA          (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 6*4)))
#define PD_CLK        (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 2*4)))
#define GREEN_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

// Masks PortD
#define DATA_MASK 64

// Masks PortE
#define PD_MASK 4

// Masks PortF
#define GREEN_LED_MASK 8;

// Global Variables
bool cycleDetected = false;
uint8_t upCount = 0;
uint8_t downCount = 0;
uint8_t ledCount = 0;
uint8_t i = 0;
uint32_t value = 0;
uint32_t prevValue = 0;
uint32_t breath_time = 0;
uint32_t timeRaw = 0;
char str[40];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void enableTimer3()
{
    // Configure Timer 1 as the time base
    TIMER3_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER3_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER3_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER3_TAILR_R = 200000000;                      // set load value to 20e7 for .2 Hz interrupt rate (5 seconds)
    TIMER3_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    TIMER3_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN1_R |= 1 << (INT_TIMER3A-16-32);             // turn-on interrupt 41 (TIMER1A)
}

void disableTimer3()
{
    TIMER3_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off time base timer
    NVIC_DIS1_R |= 1 << (INT_TIMER3A-16-32);            // turn-off interrupt 39 (TIMER2A)
}

void portDIsr(void)
{
    value = 0;
    for (i = 0; i < 24; i++)
    {
        PD_CLK = 1;
        _delay_cycles(10);
        PD_CLK = 0;
        value <<= 1;
        value |= DATA;
        _delay_cycles(10);
    }

    value = value / 10000;
    PD_CLK = 1;
    PD_CLK = 0;

    if (!cycleDetected)
    {
        timeRaw += 1;
        if (value > prevValue)
            upCount += 1;
        if (upCount == 4)
        {
            upCount = 0;
            cycleDetected = true;
        }
    }
    if (cycleDetected)
    {
        timeRaw += 1;
        if (value < prevValue)
            downCount += 1;
        if (downCount == 4)
        {
            ledCount += 1;
            GREEN_LED = 1;
            breath_time = 600/timeRaw;
            snprintf(str, sizeof(str), "Breath per min\t%"PRIu32"\n", breath_time);
            cycleDetected = false;
            downCount = 0;
            timeRaw = 0;
            TIMER3_TAILR_R = 200000000;
        }
    }
    if (ledCount > 0)
        ledCount += 1;
    if (ledCount == 11)
    {
        GREEN_LED = 0;
        ledCount = 0;
    }
    prevValue = value;

    GPIO_PORTD_ICR_R = DATA_MASK;
}

void timer3Isr()
{
    breath_time = 0;
    TIMER3_ICR_R = TIMER_ICR_TATOCINT;
}

void enablePortDISR(void)
{
    GPIO_PORTD_IS_R &= ~DATA_MASK;
    GPIO_PORTD_IBE_R &= ~DATA_MASK;
    GPIO_PORTD_IEV_R &= ~DATA_MASK;
    GPIO_PORTD_ICR_R = DATA_MASK;
    NVIC_EN0_R |= 1 << (INT_GPIOD-16);
    GPIO_PORTD_IM_R |= DATA_MASK;
}

// Initialize Hardware
void initRespHW(void)
{
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R3;

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3 | SYSCTL_RCGCGPIO_R4 | SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Enable GPIO ports
    GPIO_PORTD_DIR_R &= ~DATA_MASK;
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK;
    GPIO_PORTE_DIR_R |= PD_MASK;
    GPIO_PORTD_DEN_R |= DATA_MASK;
    GPIO_PORTE_DEN_R |= PD_MASK;
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK;

    enablePortDISR();
}
