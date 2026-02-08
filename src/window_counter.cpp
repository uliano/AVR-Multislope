
#include <avr/interrupt.h>
#include "globals.h"
#include "pins.h"
#include "negative_counter.h"
#include "tca0.h" 
#include "window_counter.h"

ISR(TCB3_INT_vect)
{
	
	ENABLE::clear();
	stop_adc_clock();
	// first thing capture LSB of TCB0 before it can increment
	window_positive.words[0] = TCB0.CNT;

	TCB0.CNT = 0;
	window_positive.bytes[2] = negative_counter_MSB;
	TCB1.CNTL = 0;

	TCB3.INTFLAGS = TCB_CAPT_bm;  // Acknowledge overflow
	reset_negative();

	window_ready = 1;
	start_adc_clock();
	ENABLE::set();

	
	
}