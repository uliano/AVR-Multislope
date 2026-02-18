/*
 * AVR128da64_board.cpp
 *
 * Created: 11/3/2022 4:35:01 PM
 * Author : uliano
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "globals.hpp"
#include "pins.hpp"
#include "clocks.h"
#include "luts.h"
#include "events.h"
#include "comparator.h"
#include "window_counter.hpp"
#include "negative_counter.hpp"
#include "heartbeat.h"
#include "timer.hpp"
#include "init.hpp"
#include "scpi.hpp"

void do_nothing() {
	;
}

Timer<Millis> nothing(1000, true, do_nothing);


int main(void)
{
	init_all();
	scpi_init();
	sei();

	nothing.start();

	while (1)
	{
		Timer<Millis>::checkAllTimers();
		scpi_service();

	}
};



