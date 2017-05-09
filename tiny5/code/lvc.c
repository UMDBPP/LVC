#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/* The purpose of this program is to prevent permanent damage of LiPo batteries
due to deep discharge where the battery is depleted beyond 80% of its capacity.

This is achieved by using MOSFET transistor as a switch.

The program will use an ADC reading coming off the battery and voltage divider
to determine the voltage. There will be three settings determined with a three
way switch: one for a standard three cell, one for standard two cell, and one
for a custom setting set with a potentiometer on the voltage divider. Thus,
the program is independent of the setting. */

/* 128 kHz (power consumption: ~ 0.02 mA at 1.8 V) */
#define F_CPU               128000UL
#define LOAD_THRESHOLD      166 /* TODO: find value for 3.25 V cutoff */
#define ADC                 PB0
#define LOAD_MOSFET         PB1 /* pin for load MOSFET */
#define LVC_MOSFET          PB2 /* pin for LVC MOSFET */

/* global variables */
volatile uint16_t sec; /* time elapsed in seconds */

int main() {
    uint16_t sec_read; /* current timer value */
    uint16_t sec_holder;
    uint8_t adc_read; /* current ADC value */
    uint8_t sreg;
    uint8_t n;

    CLKMSR = 1; /* run at 128 kHz */
    PORTB = ((1 << LOAD_MOSFET) | (1 << LVC_MOSFET)); /* keep things on */

    PRR = 0; /* power reduction module: allow timer, ADC */
    sec = 0;
    n = 0;

    /* init timer */
    /* prescalar 1024, CTC mode -> 1 second is 125 cycles */
    TCNT0 = 0;
    TCCR0A = 0;
    TCCR0B = (1 << CS02) | ( 1 << CS00) | (1 << WGM02);
    OCR0A = 0x7D; /* 125 for 1 second */
    TIMSK0 = (1 << OCIE0A);

    /* init ADC */
    /* prescalar 2, ADC0 -> pin PB0 */
    ADMUX = 0;
    ADCSRA = (1 << ADEN);
    DIDR0 = (1 << ADC0D);

    loop:
        goto get_adc;
    adc_return_0:
        if (adc_read >= LOAD_THRESHOLD) {
            goto loop;
        }
        else {
            goto get_time;
    time_return_0:
            sec_holder = sec_read + 0x5;
            n++;
    time_return_1:
            if (sec_read != sec_holder) {
                goto get_adc;
    adc_return_1:
                if (adc_read < LOAD_THRESHOLD) {
                    goto get_time;
                }
                else {
                    n = 0;
                    goto loop;
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
        TIMSK0 = 0;
        ADCSRA = 0;
        DIDR0 = 0;
        PRR = 0x3; /* power reduction: timer and adc */
        PORTB &= ~((1 << LOAD_MOSFET) | (1 << LVC_MOSFET)); /* turn all off */
        while(1); /* shutdown may not be immediate */

    return 0;
}

ISR(TIM0_COMPA_vect) {
    sec++;
}
