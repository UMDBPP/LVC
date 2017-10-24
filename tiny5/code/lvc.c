#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

/* Aravind Ramakrishnan - 24 October 2017 */

/* The purpose of this program (meant for ATTiny5 mcu) is to prevent permanent
damage of LiPo batteries due to deep discharge where the battery is depleted
beyond 80% of its capacity.

This is achieved by using MOSFET transistor as a switch.

The program will use an ADC reading coming off the battery and voltage divider
to determine the voltage. There will be three settings determined with a three
way switch: one for a standard three cell, one for standard two cell, and one
for a custom setting set with a potentiometer on the voltage divider. Thus,
the program is independent of the setting.

There will be two blinking LEDs on the device. The color will be determined by
hardware, and they will be driven by the mcu. To save power, we are blinking
as well as using PWM. */

/* 128 kHz (power consumption: ~ 50 uA at 1.8 V) */
#define F_CPU               128000UL
#define LOAD_THRESHOLD      166
#define NO_LOAD_THRESHOLD   174 /* hysteresis level */
#define ADC                 PB2
#define LOAD_MOSFET         PB1 /* pin for load MOSFET */
#define LED_OUT             PB0 /* pin for PWM on LED (OC0A) */

/* global variables */
volatile uint16_t sec; /* time elapsed in seconds */
uint8_t sec_frac; /* we operate timer in quarter seconds */

/* so we're only using some of the MCU's flash ... let's burn some more */
const uint8_t message0[] PROGMEM = {0xCA,0xFE,0xBA,0xBE}; /* this aint Java */
const uint8_t message1[] PROGMEM = "\nMade by Aravind Ramakrishnan, \
Camden Miller, and Nick Rossomando of the Univeristy of Maryland Nearspace \
Program\n";
const uint8_t message3[] PROGMEM = "32 bytes of memory ought to be enough \
for anyone!";

/* program begin */
int main() {
    uint16_t sec_read; /* current timer value */
    uint16_t sec_holder;
    uint8_t adc_read; /* current ADC value */
    uint8_t sreg;
    uint8_t cycles; /* current number of power cycles */
    uint8_t n; /* essentially a boolean saying which mode we are in */

    CLKMSR = 0x1; /* run at 128 kHz */
    CLKPSR = 0; /* clock prescalar to 1 */
    DDRB = (1 << LOAD_MOSFET);
    PORTB = (1 << LOAD_MOSFET); /* keep things on */

    PRR = 0; /* power reduction module: allow timer, ADC */
    sec = 0;
    sec_frac = 0;
    n = 0;
    cycles = 0;

    /* enable watchdog timer - 0.25 sec timeout */
    WDTCSR = 0x3; /* (1 << WDP0) | (1 << WDP1) */
    WDTCSR |= (1 << WDIE);

    /* init timer for PWM */
    /* prescalar 1024, CTC mode -> 1 second is 125 cycles */
    TCCR0A = (1 << WGM00); /* 8-bit fast PWM */
    TCCR0B = (1 << WGM02);
    TCCR0B |= (1 << CS01); /* prescalar 8 */
    OCR0A = 0x80; /* 50% duty cycle */
    DDRB |= (1 << LED_OUT); /* enable ability to use LED pin */

    /* init ADC */
    /* prescalar 2, ADC2 -> pin PB2 */
    ADMUX = 0x2;
    ADCSRA = (1 << ADEN);
    DIDR0 = (1 << ADC2D);

    sei();

    resurrect:
        PORTB |= (1 << LOAD_MOSFET);
    loop:
        goto get_adc;
    adc_return_0:
        if (adc_read >= LOAD_THRESHOLD) {
            goto loop;
        }
        else {
            PORTB &= ~(1 << LOAD_MOSFET); /* if low voltage, kill load */
            cycles++;
            if (cycles == 0xA) goto kill; /* only allow 10 power cycles */
            goto get_time;
    time_return_0:
            sec_holder = sec_read + 0x5; /* 5 second timeout */
            n++;
    time_return_1:
            if (sec_read <= sec_holder) {
                goto get_adc;
    adc_return_1:
                if (adc_read < NO_LOAD_THRESHOLD) {
                    goto get_time;
                }
                else {
                    n = 0;
                    goto resurrect;
                }
            }
            goto kill;
        }
    goto loop;

    get_adc:
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)); /* (ADCSRA & 0x40); */
        adc_read = ADCL;
        if (n == 0) goto adc_return_0;
        goto adc_return_1;

    get_time:
        sreg = SREG;
        cli();
        sec_read = sec;
        SREG = sreg;
        if (n == 0) goto time_return_0;
        goto time_return_1;

    kill:
        cli();
        TIMSK0 = 0;
        TCCR0A = 0;
        ADCSRA = 0;
        DIDR0 = 0;
        PRR = 0x3; /* power reduction: timer and adc */
        WDTCSR = 0; /* WDTCSR &= ~(1 << WDE); WDTCSR &= ~(1 << WDIE) */
        SMCR = 0x5; /* (1 << SE) | (1 << SM1) */
        __asm__("sleep");

        while(1);

    return 0;
}

ISR(WDT_vect) {
    sec_frac++;
    TCCR0A &= ~(1 << COM0A1); /* remove LED pin from output compare */
    PORTB &= ~(1 << LED_OUT);
    if (sec_frac == 0x4) {
        sec++;
        sec_frac = 0;
        TCCR0A |= (1 << COM0A1); /* blink 0.25 sec per sec */
    }
}
