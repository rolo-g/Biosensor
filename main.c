// main.c

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "adc0.h"
#include "resp.h"
#include "hrm.h"
#include "pwm1.h"
#include "uart0ext.h"
#include "tm4c123gh6pm.h"

#define RED_LED         (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define BLUE_LED        (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))

#define RED_LED_MASK 2
#define BLUE_LED_MASK 4

extern uint32_t avg;
extern uint32_t BPM;
extern bool breath_time;
extern bool pulse_active;

void initHw()
{
    initSystemClockTo40Mhz();

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    GPIO_PORTF_DIR_R |= RED_LED_MASK | BLUE_LED_MASK;
    GPIO_PORTF_DEN_R |= RED_LED_MASK | BLUE_LED_MASK;
}

void printHelp(void)
{
    putsUart0("c ON|OFF:\tprint continuously:\n");
    putsUart0("pulse|p:\tshow last pulse (if active)\n");
    putsUart0("respiration|r:\tshow last breath cycle (if active)\n");
    putsUart0("alarm pulse|breath min max:\tenable alarm for pulse/breath\n");
    putsUart0("speaker ON|OFF:\t\tdisables or enables speaker for alarm\n");
    putsUart0("clr:\tclears terminal window\n");
    putsUart0("help:\tdisplays this message again\n");
}

int main(void)
{
    initHw();
    initFingHW();
    initRespHW();
    initUart0();
    initAdc0Ss3();
    setAdc0Ss3Mux(11);
    setAdc0Ss3Log2AverageCount(2);
    enableWTimer1();
    enableTimer1();
    enableTimer2();
    enableTimer3();
    initPWM();

    char str[40];
    uint16_t speakerLoad = 15000;
    uint32_t minBPM = 0;
    uint32_t maxBPM = 0;
    uint32_t minBreath = 0;
    uint32_t maxBreath = 0;
    bool valid = true;
    bool hrmAlarm = false;
    bool respAlarm = false;
    bool printContinuously = false;
    bool alarmSwitch = false;
    USER_DATA data;
    data.fieldCount = 0;
    clearField(&data);

    putsUart0("\n");
    printHelp();

    while (true)
    {
        if (printContinuously && pulse_active)
        {
            snprintf(str, sizeof(str), "avg\t%"PRIu32" BPM\n", avg);
            putsUart0(str);
            waitMicrosecond(100000);
        }

        if (kbhitUart0())
        {
            getsUart0(&data);
            parseFields(&data);

            if (isCommand(&data, "help", 0))
            {
                printHelp();
                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "c", 1))
            {
                char* str = getFieldString(&data, 1);
                if (stringsEqual("ON", str))
                {
                    putsUart0("continuous pulse enabled\n");
                    printContinuously = true;
                    valid = true;
                }
                if (stringsEqual("OFF", str))
                {
                    putsUart0("continuous pulse disabled\n");
                    printContinuously = false;
                    valid = true;
                }
                if (!valid)
                    putsUart0("c must be set to 'ON' or 'OFF'\n");
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "pulse", 0) || isCommand(&data, "p", 0))
            {
                if (pulse_active)
                {
                    snprintf(str, sizeof(str), "%"PRIu32" BPM\n", avg);
                    putsUart0(str);
                }
                if (!pulse_active)
                    putsUart0("not detected\n");
                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "respiration", 0) || isCommand(&data, "r", 0))
            {
                if (breath_time)
                {
                    snprintf(str, sizeof(str), "%"PRIu32" breaths/minute\n", breath_time);
                    putsUart0(str);
                }
                if (!breath_time)
                    putsUart0("not detected\n");
                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "alarm", 3))
            {
                if (stringsEqual(getFieldString(&data, 1), "breath"))
                {
                    minBreath = getFieldInteger(&data, 2);
                    maxBreath = getFieldInteger(&data, 3);

                    if (minBreath == 0 && maxBreath == 0)
                    {
                        valid = true;
                        respAlarm = false;
                        putsUart0("breath alarm disabled\n");
                    }
                    else
                    {
                        snprintf(str, sizeof(str), "min\t%"PRIu32" breaths/minute\n", minBreath);
                        putsUart0(str);
                        snprintf(str, sizeof(str), "max\t%"PRIu32" breaths/minute\n", maxBreath);
                        putsUart0(str);
                        putsUart0("set min and max to 0 0 to disable alarm\n");
                        respAlarm = true;
                        valid = true;
                    }
                }
                if (stringsEqual(getFieldString(&data, 1), "pulse"))
                {
                    minBPM = getFieldInteger(&data, 2);
                    maxBPM = getFieldInteger(&data, 3);

                    if (minBPM == 0 && maxBPM == 0)
                    {
                        valid = true;
                        hrmAlarm = false;
                        putsUart0("pulse alarm disabled\n");
                    }
                    else
                    {
                        snprintf(str, sizeof(str), "min\t%"PRIu32" BPM\n", minBPM);
                        putsUart0(str);
                        snprintf(str, sizeof(str), "max\t%"PRIu32" BPM\n", maxBPM);
                        putsUart0(str);
                        putsUart0("set min and max to 0 0 to disable alarm\n");
                        hrmAlarm = true;
                        valid = true;
                    }
                }
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "speaker", 1))
            {
                char* str = getFieldString(&data, 1);
                if (stringsEqual("ON", str))
                {
                    putsUart0("speaker enabled\n");
                    speakerLoad = 15000;
                    valid = true;
                }
                if (stringsEqual("OFF", str))
                {
                    putsUart0("speaker disabled\n");
                    speakerLoad = 0;
                    valid = true;
                }
                if (!valid)
                    putsUart0("speaker must be set 'ON' or 'OFF'\n");
                clearField(&data);
                data.fieldCount = 0;
            }

            if (hrmAlarm && (BPM <= minBPM || BPM >= maxBPM))
            {
                RED_LED = 1;
                alarmSwitch = true;
            }
            else
                RED_LED = 0;
            if (respAlarm && (breath_time <= minBreath || breath_time >= maxBreath))
            {
                BLUE_LED = 1;
                alarmSwitch = true;
            }
            else
                BLUE_LED = 0;
            if (alarmSwitch)
                setPWMCMP(speakerLoad);
            else
                setPWMCMP(0);
            alarmSwitch = false;
            if (!respAlarm && !hrmAlarm)
            {
                setPWMCMP(0);
                RED_LED = 0;
                BLUE_LED = 0;
            }

            if (isCommand(&data, "clr", 0))
            {
                putsUart0("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (!valid)
            {
                putsUart0("command not valid\n");
                valid = true;
            }
            clearField(&data);
        }
    }
}
