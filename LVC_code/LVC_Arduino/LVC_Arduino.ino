// Low Voltage Cutoff Code
// Written by: William Cooper Gilbert on 11/11/2015
// Revised by: Aravind Ramakrishnan on 5/23/16
// Latest Revision: WCG on 5/24/16

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

// Pin Definitions
#define VOLTAGE_DIV A0 // analog pin for the voltage divider, used to read the value of the battery
#define LOAD 1 // digital pin for Load MOSFETs, used to allow current to flow to the load
#define ENABLE 10 // digital pin for Enable MOSFETs, used to enable voltage regulator operation
#define GREEN_LED 9 // digital pin for the Green LED, meaning the battery is charged
#define RED_LED 8 // digital pin for the Red LED, meaning the battery is depleted

// Behavioral Constants
#define LOAD_THRESHOLD 663 // battery threshold with load. 663/1023*5*3: 9.72 volts. *5 becuase this is a 5V microcontroller, *3 becuase this is a 3-cell LiPo (~3.25v per cell threshold). *** From testing, actual cutoff appears to be 9.67V.
#define NO_LOAD_THRESHOLD 720 // battery threshold with no load. 720/1023*5*3: 10.56 volts (Arbitrary, but at least 0.5v greater than Load Threshold voltage). *** From testing, actual value appears to be 10.50V.
#define WAIT_TIME 5000 // time after battery voltage drops below threshold with load before the enable pin is turned off, putting the system into ultra low power mode

// Variable declaration
int voltage; // value of the current voltage.
boolean battery_status; // battery ON/OFF
int time; // variable to keep track of runtime after battery drops below threshold

void setup() { // initializations
	battery_status = true; // declare battery initially charged
	time = 0;

	pinMode(LOAD, OUTPUT);
	pinMode(ENABLE, OUTPUT);
	pinMode(GREEN_LED, OUTPUT);
	pinMode(RED_LED, OUTPUT);
}

void loop() {
	voltage = analogRead(VOLTAGE_DIV); // update voltage variable to the current battery voltage

	if (battery_status) { // if the battery is ON
		digitalWrite(ENABLE, HIGH); // set the system to "run", the mode used during flight
		if (voltage > LOAD_THRESHOLD) { // if the battery is charged
			digitalWrite(LOAD, HIGH); // turn the Load ON
			digitalWrite(GREEN_LED, HIGH); // turn the Green LED ON
			digitalWrite(RED_LED, LOW); // turn the Red LED OFF
		}
		else { // if the battery is depleted
			digitalWrite(LOAD, LOW); // turn the Load OFF
			digitalWrite(GREEN_LED, LOW); // turn the Green LED OFF
			digitalWrite(RED_LED, HIGH); // turn the Red LED ON
			battery_status = false; // turn the battery OFF
			delay(3000); // wait three seconds for transients to settle
		}
	}
	else { // if the battery is depleted
		if (time < WAIT_TIME) { // if the inactivity countdown has not yet expired
			if (voltage < NO_LOAD_THRESHOLD) { // if the battery remains depleted
				time++; // increment the inactivity countdown
				delay(1000); // the unit of measurement of the inactivity countdown is seconds (approximately)
			}
			else { // if the battery depletion reading was in error
				battery_status = true; // reactivate the battery
				time = 0; // reset the inactivity countdown
				delay(1000); // wait one second for transitents to settle
			}
		}
		else { // if the inactivity countdown has expired
			digitalWrite(ENABLE, LOW); // put the battery into ultra low power mode
		}
	}
}
