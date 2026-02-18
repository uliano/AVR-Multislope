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

// as we use ADC to measure residual charge which is a difference between measurements
// we don't care about the reference so GND and single ended mode are perfectly fine 
// for the purpose.

static inline void init_adc(void)
{

    ADC0.CTRLA = 0;

    // mode: singel-ended, 12-bit, no free-run
    ADC0.CTRLA = 0;  

    // no accumulation (single sample)
    ADC0.CTRLB = 0;

    // ADC clock: prescaler for CLK_ADC ~ 2 MHz
    ADC0.CTRLC = ADC_PRESC_DIV12_gc; 

    // Delay 1 ADC clock before sampling to allow internal sample-and-hold to stabilize
    ADC0.CTRLD = ADC_SAMPDLY_DLY1_gc;

    // input selection: PD4(AIN4) as +, GND as -
    ADC0.MUXPOS = ADC_MUXPOS_AIN4_gc;  // input on PD4
    ADC0.MUXNEG = ADC_MUXNEG_GND_gc;

    // enable event trigger 
    ADC0.EVCTRL = ADC_STARTEI_bm;

    // interrupt on result ready
    ADC0.INTCTRL  = ADC_RESRDY_bm; 
    ADC0.INTFLAGS = ADC_RESRDY_bm; // clear pending

    // enable ADC
    ADC0.CTRLA |= ADC_ENABLE_bm; 
}

