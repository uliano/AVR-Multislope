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


// Event channel assignment.
enum {
    TCA0_OVF = 0,   // TCA0 OVF -> LUT0 & LUT1 & LUT2A clock & TCB2 count 
    WINDOW_OVF = 1,   // TCB2 OVF -> End of WINDOW
    TCB1_OVF = 2, // TCB1 OVF -> TCB2 COUNT

    AC_SYNC = 3,   // LUT2 output -> LUT0 select PWM_PATTERN
    NEG_CLK = 4,   // LUT1 output -> TCB0 count
};


static inline void init_events(void)
{
    // Event routing:
    // - CH0: TCB2 OVF -> TCB3 count
    // - CH1: TCB0 OVF -> TCB1 count
    // - CH2: TCA0 OVF -> LUT1 & LUT2A clock & TCB2 count
    // - CH3: LUT2 output -> LUT0 select PWM_PATTERN
    // - CH4: LUT1 output -> TCB0 count


    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_TCB2_CAPT_gc;
    EVSYS.CHANNEL1 = EVSYS_CHANNEL1_TCB0_OVF_gc;
    EVSYS.CHANNEL2 = EVSYS_CHANNEL2_TCA0_OVF_LUNF_gc;
    EVSYS.CHANNEL3 = EVSYS_CHANNEL3_CCL_LUT2_gc;
    EVSYS.CHANNEL4 = EVSYS_CHANNEL4_CCL_LUT1_gc;

    EVSYS.USERCCLLUT0A = (uint8_t)(CH_AC_SYNC + 1u);
    EVSYS.USERCCLLUT1A = (uint8_t)(CH_AC_SYNC + 1u); 
    EVSYS.USERCCLLUT2A = (uint8_t)(CH_TCA_OVF + 1u);  // Feeds the DFF clk
    EVSYS.USERCCLLUT4A = (uint8_t)(CH_AC_SYNC + 1u);

    EVSYS.USERTCB0COUNT = (uint8_t)(CH_POS_CLK + 1u);
    EVSYS.USERTCB1COUNT = (uint8_t)(CH_POS_OVF + 1u);
    EVSYS.USERTCB2COUNT = (uint8_t)(CH_TCA_OVF + 1u);
    EVSYS.USERTCB3COUNT = (uint8_t)(CH_MOD_OVF + 1u);

}