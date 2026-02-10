/*
 * counters.h
 *
 * Event counters for multislope window:
 * - 32-bit positive counter (TCB0+TCB1)
 * - 32-bit Modulo-N window counter (TCB2+TCB3)
 *
 * Created: 01/9/2026
 *  Author: uliano
 * Revised: 01/31/2026 Simplified to follow new hardware
 */

#pragma once
#include <avr/io.h>

/*
 * 32-bit Modulo-N Event Counter using cascaded TCB2+TCB3
 *
 * Implements a hardware event counter that generates an overflow interrupt
 * after exactly N events. 
 *
 * Architecture:
 *   Event -> TCB2 (16-bit LSW) -> cascade -> TCB3 (16-bit MSW) -> Overflow IRQ
 *   TCB2 counts up to 50, so the window counter period will be only in multiple of 50
 * 
 */



 

static inline void init_window_counter(uint16_t period)
{
    TCB2.CTRLA = 0;  // Stop TCB1 

    // Configure TCB2 for event counting (will trigger TCA0 on compare via event system)
    TCB2.CNT = 0;
    TCB2.CCMP = 24;  // the sampling period is multiple of 25
    TCB2.CTRLB = 0;  // count events
    TCB2.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB2.INTCTRL  = 0;  // Disable capture interrupt on TCB2
    TCB2.CTRLA = TCB_CLKSEL_EVENT_gc  | TCB_ENABLE_bm;  // Event mode + Enable LSW

    // Configure TCA) to count TCB2 overflows via event system
    TCA0.SINGLE.CTRLA = 0;  // Disable during configuration
    uint16_t period = static_cast<uint16_t>( (period / 25 - 1) & 0xFFFF); // 25 * 3000 = 75000 = 10 PLC
    TCA0.SINGLE.PER = period;

    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm
        | TCA_SINGLE_CMP1EN_bm
        | TCA_SINGLE_CMP2EN_bm
        | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc ;

    TCB2.CNT = 0;
    TCB2.CCMP = static_cast<uint16_t>( (period / 25 - 1) & 0xFFFF); // 25 * 3000 = 75000 = 10 PLC
    TCB2.CTRLB = 0;  // count events
    TCB2.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB2.INTCTRL  = TCB_CAPT_bm;  // Enable capture interrupt on TCB3
    TCB2.INTFLAGS = TCB_CAPT_bm;  // Clear any pending interrupt
    TCB2.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // Event mode + Enable MSW 

}


