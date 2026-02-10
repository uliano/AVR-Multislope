#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include "globals.h"

extern uint8_t negative_counter_MSB;

static inline void init_negative_counter(void)
{
    // TCB1 counts events on CH_POS (positive pulses)
    TCB1.CNT = 0;
    TCB1.EVCTRL = TCB_CAPTEI_bm;
    TCB1.INTCTRL = TCB_OVF_bm;  // No ISR; MSW handled by TCB1 via EVSYS
    TCB1.INTFLAGS = TCB_OVF_bm; // clear pending interrupt
    TCB1.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // EVENT + ENABLE
    reset_negative_counts();
}

static inline void capture_negative_counts(){
    negative_counts.words[0] = TCB1.CNT;
	negative_counts.bytes[2] = negative_counter_MSB;
    reset_negative_counts();
  
}

static inline void reset_negative_counts(void)
{
    TCB1.CNT = 0;
    negative_counter_MSB = 0;
}
