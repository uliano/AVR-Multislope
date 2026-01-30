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
	ENABLE_CLR();
	init_adc_clock();
	init_ac0();
	init_k_logic();
	init_counters();
	init_modN();
	setup_modN(7500);

	stop_counters();
	reset_counters();
	start_counters();
	start_modN();
	ENABLE_SET();
	sei();

	while (1)
	{
		if (window_ready) {
			uint8_t sreg = SREG;
			cli();
			uint32_t pos = window_positive;
			window_ready = 0;
			SREG = sreg;
			serial.print("POS=");
			serial.print(pos, 10);
			serial.print("\n");
		}
		LED_TOGGLE();
		_delay_ms(250);
	}
};



