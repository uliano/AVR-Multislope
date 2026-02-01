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
 * after exactly N events. Uses two 16-bit TCB timers in cascade mode to
 * create a 32-bit counter.
 *
 * Architecture:
 *   Event -> TCB2 (16-bit LSW) -> cascade -> TCB3 (16-bit MSW) -> Overflow IRQ
 *
 * The counter is initialized to (0 - N) so that after N events it reaches
 * 0x00000000, triggering the TCB3 overflow interrupt. This allows precise
 * period measurement or event windowing.
 */

// Pre-calculated initial values for modN counter (set by setup_modN)
static volatile uint16_t modN_init_lo;  // LSW of (0 - N)
static volatile uint16_t modN_init_hi;  // MSW of (0 - N)

static inline void init_modN(void)
{
    TCB2.CTRLA = 0;  // Stop TCB2 (LSW)
    TCB3.CTRLA = 0;  // Stop TCB3 (MSW)

    // Configure TCB2 for event counting (will trigger TCB3 on overflow via event system)
    TCB2.CTRLB = 0;  // No special mode needed, just count events
    TCB2.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB2.INTCTRL  = 0;  // No interrupts
    TCB2.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // Event mode + Enable LSW

    // Configure TCB3 to count TCB2 overflows via event system
    TCB3.CTRLB = 0;  // No special mode needed
    TCB3.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB3.INTCTRL  = TCB_OVF_bm;  // Enable overflow interrupt on TCB3
    TCB3.INTFLAGS = TCB_OVF_bm;  // Clear any pending interrupt
    TCB3.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // Event mode + Enable MSW 

}

// Reload counter to initial value (0 - N)

static inline void reload_modN(void)
{
    TCB2.CNT = modN_init_lo;        // Reload LSW
    TCB3.CNT = modN_init_hi;        // Reload MSW
}


// Stores the initial value and loads the counter

static inline void setup_modN(uint32_t N)
{
    uint32_t init = 0u - N;              // Two's complement: -N
    modN_init_lo = (uint16_t)init;       // Lower 16 bits
    modN_init_hi = (uint16_t)(init >> 16); // Upper 16 bits
    reload_modN();

}

static inline void init_positive(void)
{
    // TCB0 counts events on CH_POS (positive pulses)
    TCB0.CNT = 0;
    TCB0.EVCTRL = TCB_CAPTEI_bm;
    TCB0.INTCTRL = 0;  // No ISR; MSW handled by TCB1 via EVSYS
    TCB0.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // EVENT + ENABLE

    // TCB1 counts TCB0 overflows on CH_POS_OVF
    TCB1.CNT = 0;
    TCB1.EVCTRL = TCB_CAPTEI_bm;
    TCB1.INTCTRL = 0;  // No ISR
    TCB1.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // EVENT + ENABLE
}


// Read 32-bit counter atomically WITHOUT disabling interrupts
static inline uint32_t read_positive(void)
{
    uint16_t msb1, msb2;
    uint16_t lsw;

    do {
        msb1 = TCB1.CNT;
        lsw = TCB0.CNT;
        msb2 = TCB1.CNT;
    } while (msb1 != msb2);  // Repeat if overflow occurred during read

    return ((uint32_t)msb2 << 16) | lsw;
}



static inline void reset_positive(void)
{
    TCB0.CNT = 0;
    TCB1.CNT = 0;
}


