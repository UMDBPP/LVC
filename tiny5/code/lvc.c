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

#define F_CPU 128000UL /* 128 kHz (power consumption: ~ 0.02 mA at 1.8 V) */
#define LOAD_THRESHOLD 166 /* TODO: find value for 3.25 V cutoff */
#define NO_LOAD_THRESHOLD 180 /* arbitrary value, TODO: find value */
#define LOAD PB1 /* pin for load MOSFET */

/* global variables */
volatile uint16_t sec; /* time elapsed in seconds */

int main() {
	/* init timer */ 
	/* prescalar 1024, CTC mode -> 1 second is 125 cycles */
	sec = 0;
	TCNT0 = 0;
	TCCR0A = 0;
	TCCR0B = (1 << CS02) | ( 1 << CS00) | (1 << WGM02);
	OCR0A = 125;
	TIMSK0 = (1 << OCIE0A);

	/* init ADC */
	/* prescalar 2, ADC0 -> pin PB0 */
	PRR &= ~(1 << PRADC); /* remove power reduction to allow ADC */
	ADMUX = 0;
	ADCSRA = (1 << ADEN);
	DIDR0 = (1 << ADC0D);

	while (1) {
	
	}

	return 0;
}

ISR(TIM0_COMPA_vect) {
	sec++;
}
