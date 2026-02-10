/*
 * switching.h
 *
 * AC0 + CCL setup for synchronous K and ENABLE.
 *
 * Behavior:
 * - AC0 compares VC (AINP0 on PD2) against VREF (AINN1 on PD0).
 * - LUT2+LUT3 form a DFF that samples AC0 on TCA0 OVF (375 kHz).
 * - LUT4+LUT5 form a DFF that samples ENABLE on TCA0 OVF.
 * - LUT0 selects WO1 or WO2 based on the sampled K level.
 * - LUT1 generates gated clock = ENABLE_SYNC & TCA0 WO0.
 * - count_positive: external AND of K (PD3) and GATED_CLK (PC3) into PA2.
 *
 * Pin mappings (default CCLROUTEA):
 * - LUT2 OUT -> PD3 (K, optional debug)
 * - LUT4 OUT -> PB3 (ENABLE_SYNC), input on PB1 (software-driven)
 * - LUT0 OUT -> PA3 (selected PWM)
 * - LUT1 OUT -> PC3 (gated clock, debug and external AND input)
 */

#pragma once

#include <avr/io.h>

static inline void init_ac1(void)
{
    // AC1 compares VC (AINP0 on PD2) vs VREF (AINN1 on PD0).
    AC1.CTRLA = 0;
    AC1.CTRLB = 0;
    AC1.MUXCTRL = AC_MUXPOS_AINP0_gc | AC_MUXNEG_AINN0_gc;
    AC1.INTCTRL = 0;
    AC1.CTRLA = AC_ENABLE_bm;

}

