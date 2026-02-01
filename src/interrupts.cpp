
/*
 * interrupts.cpp
 *
 * Created: 11/3/2022 5:07:53 PM
 *  Author: uliano
 * Revised: 01/9/2026
 */ 

#include <avr/interrupt.h>
#include "globals.h"
#include "pins.h"
#include "counters.h"
#include "tca0.h"

ISR(USART2_RXC_vect) {
	serial.rxc();
}

ISR(USART2_DRE_vect) {
	serial.dre();
}

ISR(TCB3_INT_vect)
{
	ENABLE::clear();
	stop_adc_clock();
	TCB3.INTFLAGS = TCB_OVF_bm;  // Acknowledge overflow

	window_positive = read_positive();
	window_ready = 1;
	
	reload_modN();
	reset_positive();
}
