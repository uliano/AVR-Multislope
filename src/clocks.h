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
// 2 = RTC oscillator (32768 Hz) failed to start
// 3 = external clock failed (using internal) + RTC failed

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

