
/*
 * interrupts.cpp
 *
 * Created: 11/3/2022 5:07:53 PM
 *  Author: uliano
 * Revised: 01/9/2026
 */ 

#include <avr/interrupt.h>
#include "globals.h"


ISR(USART2_RXC_vect) {
	serial.rxc();
}

ISR(USART2_DRE_vect) {
	serial.dre();
}


