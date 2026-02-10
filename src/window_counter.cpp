
#include <avr/interrupt.h>
#include "globals.h"
#include "pins.h"
#include "negative_counter.h"
#include "tca0.h" 
#include "window_counter.h"

ISR(TCB_INT_vect)
{
	
	ENABLE::clear();
	stop_adc_clock();
	// first thing capture LSB of TCB0 before it can increment
	capture_negative_counts();
	TCB2.INTFLAGS = TCB_CAPT_bm;  // Acknowledge overflow
	window_ready = 1;
	start_adc_clock();
	ENABLE::set();

	
	
}