#pragma once
#include <avr/io.h>

// by setting the negative reference to the midpoint between VREF and GND we choose 
// to operate the integrator in the positive voltage region (eventual negative are clipped by
// diode clamp) so that the internal ADC can be used without voltage shifting.

static inline void init_ac1(void)
{
    // AC1 compares VC (AINP0 on PD2) vs VREF (AINN1 on PD0).
    AC1.CTRLA = 0;
    AC1.CTRLB = 0;
    AC1.MUXCTRL = AC_MUXPOS_AINP2_gc | AC_MUXNEG_DACREF_gc;  // input on PD4
    AC1.DACREF = 0x7F; // Midpoint reference
    AC1.INTCTRL = 0;
    AC1.CTRLA = AC_ENABLE_bm;

}

