/* 
 * clocks.h
 * 
 * System clock initialization
 * 
 * Created: 01/9/2026 
 *  Author: uliano 
 */

#pragma once
#include <avr/io.h>

// Return codes for init_clocks:
// 0 = success with external clock
// 1 = external clock (24 MHz) failed, using internal oscillator
// 2 = crystal oscillator (32768 Hz) failed to start
// 3 = external clock failed (using internal) + crystal failed

#ifndef USE_EXTERNAL_CLOCK
#define USE_EXTERNAL_CLOCK 1
#endif

static uint8_t init_clocks(void)
{
    uint8_t error_code = 0;
    uint32_t timeout;

    // Step 1: Start with internal oscillator at 24 MHz
    _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc | CLKCTRL_RUNSTDBY_bm);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSCHF_gc);

    // Wait for internal oscillator to be active
    timeout = 0xFFFFF;
    while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm) && timeout--) {
        // Wait for internal oscillator
    }

    // Step 2: Start crystal 32768 Hz on PF0, PF1
    _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA, CLKCTRL_ENABLE_bm
        | CLKCTRL_RUNSTDBY_bm
        | CLKCTRL_LPMODE_bm
        | CLKCTRL_CSUT_1K_gc
        // | CLKCTRL_SEL_bm // External clock, no xtal
    );

    // Wait for 32kHz crystal to stabilize with timeout
    timeout = 0xFFFFF;
    while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm) && timeout--) {
        // Wait for 32kHz crystal to be stable
    }

    bool xtal_32k_ok = (CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm);
    if (!xtal_32k_ok) {
        error_code |= 2;  // 32kHz crystal failed
    }

#if USE_EXTERNAL_CLOCK
    // Step 3: Try external clock 24 MHz on PA0
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_EXTCLK_gc);

    // Wait for external clock to be ready
    timeout = 0xFFFF;
    while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm) && timeout--) {
        // Wait for external clock source
    }

    if (!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm)) {
        // Step 4: External clock not available, go back to internal
        error_code |= 1;

        // If we have the 32kHz crystal, enable autotune
        if (xtal_32k_ok) {
            _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc
                | CLKCTRL_RUNSTDBY_bm
                | CLKCTRL_AUTOTUNE_bm);
        }

        // Switch back to internal oscillator
        _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSCHF_gc);

        // Wait for switch to complete
        timeout = 0xFFFF;
        while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm) && timeout--) {
            // Wait for internal oscillator
        }
    }
#endif

    return error_code;
}

/**
 * @brief Generate ~375 kHz on TCA0 using CLK_PER=24 MHz
 *
 * Outputs:
 * - WO0 -> PC0 (50%)
 * - WO1 -> PC1 (CMP1=7)
 * - WO2 -> PC2 (CMP2=55)
 */
static inline void init_adc_clock()
{
    // Route TCA0 outputs to PORTC (WO0→PC0, WO1→PC1, WO2→PC2)
    PORTMUX.TCAROUTEA = (PORTMUX.TCAROUTEA & ~0x07) | PORTMUX_TCA0_PORTC_gc;

    TCA0.SINGLE.CTRLA = 0;  // Disable during configuration
    TCA0.SINGLE.PER = 63;   // 64 clock cycles = 375 kHz
    TCA0.SINGLE.CMP0 = 31;  // ~50% duty on WO0 (PC0)
    TCA0.SINGLE.CMP1 = 7;   // ~1/8 duty on WO1 (PC1)
    TCA0.SINGLE.CMP2 = 55;  // ~7/8 duty on WO2 (PC2)
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm
        | TCA_SINGLE_CMP1EN_bm
        | TCA_SINGLE_CMP2EN_bm
        | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;
}
