/*
 * AVR128da64_board.cpp
 *
 * Created: 11/3/2022 4:35:01 PM
 * Author : uliano
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "globals.h"
#include "pins.h"
#include "clocks.h"
#include "switching.h"
#include "counters.h"
#include "tca0.h"

int main(void)
{
	uint8_t clock_status = init_clocks();

	if (clock_status == 0) {
		serial.print("clocks up\n");
	} else {
		serial.print("Clock init error: ");
		if (clock_status & 1) {
			serial.print("External 24MHz clock failed. ");
		}
		if (clock_status & 2) {
			serial.print("32kHz crystal failed. ");
		}
		serial.print("\n");
	}
	init_pins();
	init_adc_clock();
	init_ac1();
	init_luts();
	init_modN(7500);
	init_positive();
	reset_positive();
	sei();
	ENABLE::set();
	start_adc_clock();

	while (1)
	{

		
		if (window_ready) {
			// DEBUG::set();
			// DEBUG::clear();
			window_ready = 0;
			//start_adc_clock();
			serial.print("NEG=");
			serial.print(window_positive.value, 10);
			serial.print("\n");

			// serial.print("TCB2CMP=");
			// serial.print(TCB2.CCMP,10);
			// serial.print("\n");
			// serial.print("TCB3CMP=");
			// serial.print(TCB3.CCMP,10);
			// serial.print("\n");

			
			// ENABLE::set();
			// start_adc_clock();

		}

	}
};



