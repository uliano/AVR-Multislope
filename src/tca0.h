#pragma once
#include <avr/io.h>

/**
 * @brief Generate ~375 kHz on TCA0 using CLK_PER=24 MHz
 *
 * Outputs for debug puroposes:
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
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc ;
}

static inline void start_adc_clock() {
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
}

static inline void stop_adc_clock() {
    TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;
}

static inline void set_adc_clock(uint8_t value) {
    TCA0.SINGLE.CNTL = value;
}

