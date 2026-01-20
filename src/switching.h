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
    CH_POS_EXT = 0,   // PA2 -> TCB0 count
    CH_POS_OVF = 1,   // TCB0 OVF -> TCB1 count
    CH_TCA_OVF = 2,   // TCA0 OVF -> LUT2A/LUT4A clock
    CH_K = 3,         // LUT2 output -> LUT0 select
    CH_EN_SYNC = 4,   // LUT4 output -> LUT1 gate
    CH_WIN_PULSE = 5, // LUT1 output -> TCB2 count
    CH_MOD_OVF = 6    // TCB2 OVF -> TCB3 count
};

static inline void init_ac0(void)
{
    // AC0 compares VC (AINP0 on PD2) vs VREF (AINN1 on PD0).
    AC0.CTRLA = 0;
    AC0.CTRLB = 0;
    AC0.MUXCTRL = AC_MUXPOS_AINP0_gc | AC_MUXNEG_AINN1_gc;
    AC0.INTCTRL = 0;
    AC0.CTRLA = AC_ENABLE_bm;

    // Disable digital input buffer on comparator pins to reduce noise/power.
    PORTD.PIN0CTRL = (PORTD.PIN0CTRL & ~PORT_ISC_gm) | PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN2CTRL = (PORTD.PIN2CTRL & ~PORT_ISC_gm) | PORT_ISC_INPUT_DISABLE_gc;
}

static inline void init_k_logic(void)
{
    // Event routing:
    // - CH0: PA2 -> TCB0 count (external AND of K & GATED_CLK)
    // - CH1: TCB0 OVF -> TCB1 count
    // - CH2: TCA0 OVF -> LUT2A/LUT4A (DFF clocks)
    // - CH3: LUT2 output -> LUT0A (K select)
    // - CH4: LUT4 output -> LUT1A (ENABLE_SYNC)
    // - CH5: LUT1 output -> TCB2 count
    // - CH6: TCB2 OVF -> TCB3 count
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN2_gc;
    EVSYS.CHANNEL1 = EVSYS_CHANNEL1_TCB0_OVF_gc;
    EVSYS.CHANNEL2 = EVSYS_CHANNEL2_TCA0_OVF_LUNF_gc;
    EVSYS.CHANNEL3 = EVSYS_CHANNEL3_CCL_LUT2_gc;
    EVSYS.CHANNEL4 = EVSYS_CHANNEL4_CCL_LUT4_gc;
    EVSYS.CHANNEL5 = EVSYS_CHANNEL5_CCL_LUT1_gc;
    EVSYS.CHANNEL6 = EVSYS_CHANNEL6_TCB2_OVF_gc;
    EVSYS.CHANNEL7 = 0;
    EVSYS.CHANNEL8 = 0;
    EVSYS.USERCCLLUT0A = (uint8_t)(CH_K + 1u);
    EVSYS.USERCCLLUT1A = (uint8_t)(CH_EN_SYNC + 1u);
    EVSYS.USERCCLLUT2A = (uint8_t)(CH_TCA_OVF + 1u);
    EVSYS.USERCCLLUT4A = (uint8_t)(CH_TCA_OVF + 1u);
    EVSYS.USERTCB2COUNT = (uint8_t)(CH_WIN_PULSE + 1u);
    EVSYS.USERTCB3COUNT = (uint8_t)(CH_MOD_OVF + 1u);
    EVSYS.USERTCB0COUNT = (uint8_t)(CH_POS_EXT + 1u);
    EVSYS.USERTCB1COUNT = (uint8_t)(CH_POS_OVF + 1u);
    EVSYS.USEREVSYSEVOUTB = (uint8_t)(CH_WIN_PULSE + 1u);  // PB2 = gated pulses

    // Disable CCL while configuring
    CCL.CTRLA = 0;

    // PA2 is POS_EXT input from external AND.
    // Keep PA2 pin interrupts disabled to avoid starving UART IRQs.
    // EVSYS still sees port pin edges without enabling the PORT interrupt.
    PORTA.PIN2CTRL = (PORTA.PIN2CTRL & ~PORT_ISC_gm) | PORT_ISC_INTDISABLE_gc;

    // LUT0: MUX between WO1 and WO2 based on K (IN0)
    // IN0 = EVENTA (K), IN1 = TCA0 WO1, IN2 = TCA0 WO2
    CCL.SEQCTRL0 = CCL_SEQSEL_DISABLE_gc;
    CCL.LUT0CTRLB = CCL_INSEL0_EVENTA_gc | CCL_INSEL1_TCA0_gc;
    CCL.LUT0CTRLC = CCL_INSEL2_TCA0_gc;
    CCL.TRUTH0 = 0xD8;  // OUT = IN0 ? IN1 : IN2
    CCL.LUT0CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;  // Output on PA3

    // LUT1: gated clock = ENABLE_SYNC & TCA0 WO0 (debug on PC3)
    // IN0 = TCA0 WO0, IN1 = EVENTA (ENABLE_SYNC), IN2 masked (0)
    CCL.LUT1CTRLB = CCL_INSEL0_TCA0_gc | CCL_INSEL1_EVENTA_gc;
    CCL.LUT1CTRLC = CCL_INSEL2_MASK_gc;
    CCL.TRUTH1 = 0x08;  // OUT = IN0 & IN1 (IN2=0)
    CCL.LUT1CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;

    // LUT2+LUT3: DFF synchronized to 375 kHz clock
    // Q = AC0 sampled on rising edge of clock (anti-jitter)
    CCL.SEQCTRL1 = CCL_SEQSEL_DFF_gc;  // Enable DFF sequencer for LUT2+LUT3

    // LUT2: D input of DFF (AC0 comparator), clock from IN2 (Event A)
    CCL.LUT2CTRLB = CCL_INSEL0_AC0_gc | CCL_INSEL1_MASK_gc;
    CCL.LUT2CTRLC = CCL_INSEL2_EVENTA_gc;  // Clock from TCA0 OVF (EVSYS)
    CCL.TRUTH2 = 0xAA;  // Copy IN0 to D input of DFF
    CCL.LUT2CTRLA = CCL_CLKSRC_IN2_gc | CCL_OUTEN_bm | CCL_ENABLE_bm;  // K on PD3 (debug)

    // LUT3: G input of DFF (always enabled)
    CCL.LUT3CTRLB = CCL_INSEL0_MASK_gc | CCL_INSEL1_MASK_gc;
    CCL.LUT3CTRLC = CCL_INSEL2_MASK_gc;
    CCL.TRUTH3 = 0xFF;  // Always 1 (G=1 enables DFF)
    CCL.LUT3CTRLA = CCL_ENABLE_bm;  // No output pin needed

    // LUT4+LUT5: DFF synchronized to 375 kHz clock for ENABLE
    // D = PB1 (IN1), clock on EVENTA (TCA0 OVF), Q on PB3
    CCL.SEQCTRL2 = CCL_SEQSEL_DFF_gc;
    CCL.LUT4CTRLB = CCL_INSEL0_MASK_gc | CCL_INSEL1_IN1_gc;
    CCL.LUT4CTRLC = CCL_INSEL2_EVENTA_gc;
    CCL.TRUTH4 = 0xCC;  // OUT = IN1 (ENABLE)
    CCL.LUT4CTRLA = CCL_CLKSRC_IN2_gc | CCL_OUTEN_bm | CCL_ENABLE_bm;

    // LUT5: G input of DFF (always enabled)
    CCL.LUT5CTRLB = CCL_INSEL0_MASK_gc | CCL_INSEL1_MASK_gc;
    CCL.LUT5CTRLC = CCL_INSEL2_MASK_gc;
    CCL.TRUTH5 = 0xFF;  // Always 1
    CCL.LUT5CTRLA = CCL_ENABLE_bm;  // No output pin

    // Route LUT0/LUT2/LUT4 outputs to default pins (PA3, PD3, PB3)
    PORTMUX.CCLROUTEA &= (uint8_t)~(PORTMUX_LUT0_bm | PORTMUX_LUT2_bm | PORTMUX_LUT4_bm);

    // Enable CCL
    CCL.CTRLA = CCL_ENABLE_bm;
}
