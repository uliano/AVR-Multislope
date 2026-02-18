
/*
 * interrupts.cpp
 *
 * Created: 11/3/2022 5:07:53 PM
 *  Author: uliano
 * Revised: 01/9/2026
 */ 

#include <avr/interrupt.h>
#include <avr/io.h>
#include "globals.hpp"
#include "ticker.hpp"
#include "negative_counter.hpp"
#include "status.h"


ISR(RTC_PIT_vect) {
	Ticker::ptr->pit();
}


ISR(USART2_RXC_vect) {
	usb.rxc();
}

ISR(USART2_DRE_vect) {
	usb.dre();
}

ISR(USART4_RXC_vect) {
	console.rxc();
}

ISR(USART4_DRE_vect) {
	console.dre();
}


ISR(TCB1_INT_vect) {
	negative_counter.isr();
}


ISR(TCB3_INT_vect)
{
	window_counter.isr();
}

ISR(ADC0_RESRDY_vect) {
	ADC0.INTFLAGS = ADC_RESRDY_bm; // Clear interrupt flag
	int16_t adc_result = static_cast<int16_t> (ADC0.RES); // Read ADC result to clear the conversion complete flag
	switch (globals->status) {
		case Status::CLEAN:
			globals->previous_charge = adc_result;
			globals->status = Status::PREV_CHARGE;
			break;
		case Status::PREV_CHARGE:
		// todo trigger error condition to be implemented with LED
			break;
		case Status::NEGATIVE_COUNTS:
			globals->charge_difference = adc_result - globals->previous_charge;
			globals->previous_charge = adc_result;
			globals->status = Status::RESULT_AVAIL;	
			break;
		case Status::RESULT_AVAIL:
		// todo trigger error condition to be implemented with LED
			break;
		default:
			break;
	}
}

