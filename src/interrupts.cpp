
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
	// first thing capture LSB of TCB0 before it can increment
	window_positive.words[0] = TCB0.CNT;

	TCB0.CNT = 0;
	window_positive.bytes[2] = TCB1.CNTL;
	TCB1.CNTL = 0;

	TCB3.INTFLAGS = TCB_CAPT_bm;  // Acknowledge overflow

	window_ready = 1;
	start_adc_clock();
	ENABLE::set();

	
	
	// reset_positive();
}
