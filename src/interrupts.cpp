
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
#include "switching.h"
#include "counters.h"

ISR(USART2_RXC_vect) {
	serial.rxc();
}

ISR(USART2_DRE_vect) {
	serial.dre();
}

ISR(TCB3_INT_vect)
{
	TCB3.INTFLAGS = TCB_OVF_bm;  // Acknowledge overflow

	PORTF.OUTTGL = PIN2_bm;  // Debug: toggle PF2 on each modN overflow

	ENABLE::clear();
	stop_modN();
	stop_counters();

	// Read 32-bit positive counter (TCB1:MSW, TCB0:LSW)
	uint16_t msw1;
	uint16_t msw2;
	uint16_t lsw;
	do {
		msw1 = TCB1.CNT;
		lsw = TCB0.CNT;
		msw2 = TCB1.CNT;
	} while (msw1 != msw2);

	window_positive = ((uint32_t)msw2 << 16) | lsw;
	window_ready = 1;

	reset_counters();
	reload_modN();
	start_counters();
	start_modN();
	ENABLE::set();
}

ISR(TCB2_INT_vect)
{
	TCB2.INTFLAGS = TCB_OVF_bm;  // Acknowledge overflow
	PORTC.OUTTGL = PIN4_bm;      // Debug: toggle PC4 on each TCB2 overflow
}
