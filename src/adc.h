/*
 * adc.h
 *
 * ADC peripheral configuration (not wired into the flow yet)
 *
 * Created: 01/9/2026
 *  Author: uliano
 */

#pragma once

#include <avr/io.h>

static inline void adc_init(void)
{
    // 1) disable ADC while configuring
    ADC0.CTRLA = 0;

    // 2) disable digital input buffer on ADC pins (power/noise)
    // PD1 = AIN1, PD4 = AIN4
    PORTD.PIN1CTRL = (PORTD.PIN1CTRL & ~PORT_ISC_gm) | PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN4CTRL = (PORTD.PIN4CTRL & ~PORT_ISC_gm) | PORT_ISC_INPUT_DISABLE_gc;

    // 3) mode: differential, 12-bit, no free-run
    // CTRLA: CONVMODE=DIFF, RESSEL=12bit (0), ENABLE lo mettiamo alla fine
    ADC0.CTRLA = ADC_CONVMODE_bm;  // Differential mode

    // 4) no accumulation (single sample)
    ADC0.CTRLB = 0;

    // 5) ADC clock: prescaler for CLK_ADC ~ 2 MHz
    // Se CLK_PER=24 MHz => PRESC=DIV12 dà 2 MHz.
    // Il nome esatto dell'enum dipende dall'header; spesso è ADC_PRESC_DIV12_gc.
    ADC0.CTRLC = ADC_PRESC_DIV12_gc; 

    // 6) input selection: PD1(AIN1) as +, PD4(AIN4) as -
    ADC0.MUXPOS = 1;  // AIN1
    ADC0.MUXNEG = 4;  // AIN4

    // 7) interrupt on result ready
    ADC0.INTCTRL  = ADC_RESRDY_bm; 
    ADC0.INTFLAGS = ADC_RESRDY_bm; // clear pending

    // 8) enable ADC
    ADC0.CTRLA |= ADC_ENABLE_bm; 
}

static inline void adc0_start(void)
{
    ADC0.COMMAND = ADC_STCONV_bm;
}
