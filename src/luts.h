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


static inline void init_luts(void)
{

    // Disable CCL while configuring
    CCL.CTRLA = 0;

    // LUT0: MUX between WO1 and WO2 based on AC_SYNC (IN0)
    // IN0 = EVENTA (AC_SYNC), IN1 = TCA0 WO1, IN2 = TCA0 WO2
    CCL.SEQCTRL0 = CCL_SEQSEL_DISABLE_gc;
    CCL.LUT0CTRLB = CCL_INSEL0_EVENTA_gc | CCL_INSEL1_TCA0_gc; // AC_SYNC; WO1
    CCL.LUT0CTRLC = CCL_INSEL2_TCA0_gc; // WO2
    CCL.TRUTH0 = 0xD8;//0xD8;// 0xE4; // 0xD8;  // OUT = IN0 ? IN1 : IN2
    CCL.LUT0CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;  // Output on PA3

    // LUT1: positive clock = AC_Sync & TCA0 WO0 (debug on PC3)
    // IN0 = TCA0 WO0, IN1 = EVENTA (ENABLE_SYNC), IN2 masked (0)
    CCL.LUT1CTRLB = CCL_INSEL0_TCA0_gc | CCL_INSEL1_EVENTA_gc;
    CCL.LUT1CTRLC = CCL_INSEL2_MASK_gc;
    CCL.TRUTH1 = 0x08;  // OUT = IN0 & IN1 (IN2=0)
    CCL.LUT1CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;

    // LUT2+LUT3: DFF synchronized to 375 kHz clock
    // Q = AC1 sampled on rising edge of clock 
    CCL.SEQCTRL1 = CCL_SEQSEL_DFF_gc;  // Enable DFF sequencer for LUT2+LUT3

    // LUT2: D input of DFF (AC1 comparator), clock from IN2 (Event A)
    CCL.LUT2CTRLB = CCL_INSEL0_MASK_gc | CCL_INSEL1_AC1_gc;
    CCL.LUT2CTRLC = CCL_INSEL2_EVENTA_gc;  // Clock from TCA0 OVF (EVSYS)
    CCL.TRUTH2 = 0xCC;  // Copy IN1 to D input of DFF
    CCL.LUT2CTRLA = CCL_CLKSRC_IN2_gc | CCL_OUTEN_bm | CCL_ENABLE_bm;  // Output AC_SYNC on PD3 (debug)

    // LUT3: G input of DFF (always enabled)
    CCL.LUT3CTRLB = CCL_INSEL0_MASK_gc | CCL_INSEL1_MASK_gc;
    CCL.LUT3CTRLC = CCL_INSEL2_MASK_gc;
    CCL.TRUTH3 = 0xFF;  // Always 1 (G=1 enables DFF)
    CCL.LUT3CTRLA = CCL_ENABLE_bm;  // No output pin needed

    // Route LUT0/LUT1/LUT2 outputs to default pins (PA3, PC3, PD3)
    PORTMUX.CCLROUTEA &= (uint8_t)~(PORTMUX_LUT0_bm | PORTMUX_LUT1_bm | PORTMUX_LUT2_bm);

    // Enable CCL
    CCL.CTRLA = CCL_ENABLE_bm;
}
