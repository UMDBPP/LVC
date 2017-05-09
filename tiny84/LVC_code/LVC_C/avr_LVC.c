// Low Voltage Cutoff Code
// Written by: William Cooper Gilbert on 11 November, 2015
// Revised by: Aravind Ramakrishnan on 23 May, 2016
// Revised by: William Cooper Gilbert on 24 May, 2016
// Rewritten by: Aravind Ramakrishnan on 26 June, 2016 (from Arduino to AVR C)
// Revised by Aravind Ramakrishnan on 10 November 2016

/*
PURPOSE
The purpose of this program is to prevent permanent damage of lithium polymer (LiPo) flight batteries due to deep discharge, where the battery is depleted beyond 80% of its
stated capacity, as indicated by the voltage dropping below 3.25v per cell. This is achieved by using MOSFETs as swtiches, with the gates controlled by an AT-Tiny 84 micro-
controller. When the flight battery in question is above the designated 3.25v per cell voltage, as read by the microcontroller, the gates are set to allow current to flow
to the load, and a green LED is used to show that the battery is above the threshold. When the battery voltage drops below 3.25v per cell, the gates are set to turn off the
current to the load, a red LED is turned on to indicate this visually, and a countdown begun, for ~1.5 hours. If the battery dropped below 3.25v per cell for only a brief time,
the system will turn back on, current will flow to the load, and the green LED will be reactivated. However, it the battery is genuinely depleted, and remains depleted for ~1.5 hours,
the microcontroller will turn off the enable pin that allows the voltage regulator powering the microcontroller to operate, shutting the entire system down except for the
quiescent current flowing through the regulator. This state is called ultra low power mode. A blue LED is used to indicate the status of the enable pin on the regulator: the blue 
LED being on indicates that the regulator is enabled, while the blue LED being off indicates that the regulator is not enabled. A toggle switch on the board allows the blue LED
to be disconnected to save power during flight.

HARDWARE
Surface-mount Device (SMD) Low Voltage Cutoff (LVC) Circuit Board using AT-Tiny 84 microcontroller

OPERATION
Set switches to RUN and LED ON, and connect battery, observe all LEDs are off
Set switch to START, observe blue and green LEDs on simultaneously
Set switch to LED OFF, observe only green LED on
Set switch to RUN, observe only the green LED on
 */

/* Includes */
#include <avr/io.h>
#include <util/delay.h>

/* Pin Definitions */
#define VOLTAGE_DIV PA0 // pin for the voltage divider, used to read the value of the battery. Use in analog mode.
#define LOAD PA1 // pin for Load MOSFETs, used to allow current to flow to the load
#define ENABLE PB0 // digital pin for Enable MOSFETs, used to enable voltage regulator operation
#define GREEN_LED PB1 // digital pin for the Green LED, meaning the battery is charged
#define RED_LED PB2 // digital pin for the Red LED, meaning the battery is depleted

/* Behavioral Constants */
#ifndef CELLS
	#define CELLS 2 // default to 2 cells. Compile time input for cell number
#endif
// Note that the inconsistencies in values (eg. 675 instead of 663) is from testing values
#define LOAD_THRESHOLD ((uint16_t)(CELLS * 675 / 3)) // battery threshold with load. 663/1023*5*3: 9.72 volts. *5 becuase this is a 5V microcontroller, *3 becuase this is a 3-cell LiPo (~3.25v per cell threshold).
#define NO_LOAD_THRESHOLD ((uint16_t)(CELLS * 715 / 3)) // battery threshold with no load. 700/1023*5*3: 10.26 volts (Arbitrary, but at least 0.5v greater than Load Threshold voltage). *** From testing, actual value appears to be 10.50V.
#define WAIT_TIME 5000 // time after battery voltage drops below threshold with load before the enable pin is turned off, putting the system into ultra low power mode

/* Function Prototypes */
void init_ADC(); // for setting up the analog conversion on the voltage divider pin
int read_ADC(); // read the voltage divider pin
void work(); // normal execution
void halt(char *battery_status); // battery is now depleted, so turn load off, set LED output
void wait(int *time); // while battery is depleted, increment timer to check for timeout and wait
void resurrect(char *battery_status, int *time); // if battery depletion reading was in error, continue executing normally
void kill(); // after timeout, put into ultra low power mode

int main() {
	/* Program Variables */
	int voltage; // value of current voltage.
	char battery_status = 'a'; // Battery on/off. On = 'a', OFF = 'd'.
	int time = 0; // variable to keep track of runtime after battery drops below threshold.

	/* Enable Pins */
	DDRA |= (1 << LOAD);
	DDRB |= (1 << ENABLE);
	DDRB |= (1 << GREEN_LED);
	DDRB |= (1 << RED_LED);
	init_ADC();

	/* Execution Block */
	while (1) {
		voltage = read_ADC();
		if (battery_status == 'a') { // if battery is ON
			PORTB |= (1 << ENABLE); // set system to "run", the mode used during flight
			if (voltage > LOAD_THRESHOLD) { // if battery is charged
				work();
			}
			else { // if battery is depleted
				halt(&battery_status);
			}
		}
		else {
			if (time < WAIT_TIME) { // if inactivity countdown has not yet expired
				if (voltage < NO_LOAD_THRESHOLD) { // if battery remains depleted
					wait(&time);
				}
				else { // if battery depletion reading was in error
					resurrect(&battery_status, &time);
				}
			}
			else { // if inactivity countdown has expired
				kill();
			}
		}
	}
	return 0; // should never be reached
}

void init_ADC() {
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0); // set prescalar to 16
	// ADMUX is fine as it is - PA0 is set with 5:0 set to 0, and to use Vcc, REFS{0,1} should be 0
	DIDR0 |= (1 << ADC0D); // disable digital input
	// For ATTiny84, there is no free running mode
	ADCSRA |= (1 << ADEN); // enable analog digital conversion
}

int read_ADC() {
	ADCSRA |= (1 << ADSC); // start analog digital conversion
	while (ADCSRA & (1 << ADSC)); // wait for read to finish
	return ADC; // return analog value: Vin * 1024 / Vref. (Vref is 5V)
}

void work() {
	PORTA |= (1 << LOAD); // turn load ON
	PORTB |= (1 << GREEN_LED); // turn green LED ON
	PORTB &= ~(1 << RED_LED); // turn red LED OFF
}

void halt(char *battery_status) {
	PORTA &= ~(1 << LOAD); // turn the load OFF
	PORTB &= ~(1 << GREEN_LED); // turn the green LED OFF
	PORTB |= (1 << RED_LED); // turn the red LED ON
	*battery_status = 'd'; // turn battery status to disabled
	_delay_ms(3000); // wait three seconds for transients to settle
}

void wait(int *time) {
	(*time)++; // increment inactivity countdown
	_delay_ms(1000); // inactivity countdown is by seconds
}

void resurrect(char *battery_status, int *time) {
	*battery_status = 'a'; // battery status to active
	*time = 0;
	_delay_ms(3000); // wait three seconds for transients to settle
}

void kill() {
	PORTB &= ~(1 << ENABLE); // put the battery into ultra low power mode
}
