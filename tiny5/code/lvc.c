#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/* The purpose of this program is to prevent permanent damage of LiPo batteries due to deep discharge where the battery is depleted beyond 80% of its capacity.

This is achieved by using MOSFET transistor as a switch.

The program will use an ADC reading coming off the battery and voltage divider to determine the voltage. There will be three settings determined with a three way switch: one for a standard three cell, one for standard two cell, and one for a custom setting set with a potentiometer on the voltage divider. Thus, the program is independent of the setting. */

#define F_CPU 128000UL /* 128 kHz (power consumption: ~ 0.02 mA at 1.8 V) */

int main() {
	/* init timer */
	

	/* init ADC */


	while (1) {
	
	}

	return 0;
}
