#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>

extern uint8_t negative_counter_MSB;

static inline void init_negative(void)
{
    // TCB0 counts events on CH_POS (positive pulses)
    TCB0.CNT = 0;
    TCB0.EVCTRL = TCB_CAPTEI_bm;
    TCB0.INTCTRL = TCB_OVF_bm;  // No ISR; MSW handled by TCB1 via EVSYS
    TCB0.INTFLAGS = TCB_OVF_bm; // clear pending interrupt
    TCB0.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // EVENT + ENABLE

}



static inline void reset_negative(void)
{
    TCB0.CNT = 0;
    negative_counter_MSB = 0;
}
