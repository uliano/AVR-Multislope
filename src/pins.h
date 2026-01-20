/*
 * pins.h
 *
 * Pin assignment and macros using X-Macro pattern
 *
 * To add a new output pin:
 * 1. Add it to PIN_TABLE: PIN_DEF(NAME, PORT_LETTER, PIN_NUMBER)
 * 2. That's it! Macros NAME_TOGGLE, NAME_SET, NAME_CLR are auto-generated
 *
 * Created: 01/9/2026
 *  Author: uliano
 * Revised: 01/9/2026 - Converted to X-Macro pattern
 */

#pragma once
#include <avr/io.h>

// pin definitions

// PORT A

// PA0 (INPUT) External 24 MHz clock input - DO NOT USE
// PA1 (INPUT)  UNUSED (PCB issue: do not use as output)
// PA2 (INPUT)  POS_EXT (AND of K & GATED_CLK)
// PA3 (OUTPUT) MUX_OUT (LUT0 output, CCL default mapping)


// PORT B

// PB1 (OUTPUT PP)  ENABLE (software-controlled)
// PB2 (OUTPUT PP)  EVOUTB debug (gated clock pulses)
// PB3 (OUTPUT PP)  ENABLE_SYNC (LUT4 output)


// PORT C

// PC0 (OUTPUT PP)  ADC clock 375 kHz (TCA0 WO0, PORTMUX→PORTC)
// PC1 (OUTPUT PP)  PWM (TCA0 WO1, CMP1=7)
// PC2 (OUTPUT PP)  PWM (TCA0 WO2, CMP2=55)
// PC3 (OUTPUT PP)  GATED_CLK (LUT1 output, debug and AND input)
// PC4 (OUTPUT PP)  TCB2 overflow debug (GPIO toggle)

// PORT D
// PD0 (ANALOG IN)  VREF (AC0 negative input)
// PD1 (ANALOG IN)  ADC+ (AIN1)
// PD2 (ANALOG IN)  VC (AC0 positive input)
// PD3 (OUTPUT PP)  K (CCL LUT2 output, debug)
// PD4 (ANALOG IN)  ADC- (AIN4)



// PORT E

// PE0 (INPUT)      free (unused)
// PE1 (OUTPUT PP)  TICK
// PE2 (OUTPUT PP)  DEBUG


// PORT F

// PF0 (INPUT)      X32k1
// PF1 (INPUT)      X32k2
// PF2 (OUTPUT PP)  LOOP (ISR debug toggle)
// PF3 (OUTPUT PP)  LED
// PF4 (OUTPUT PP)  UART TX
// PF5 (INPUT)      UART RX
// PF6 (INPUT)      RESET*



// Pin configuration table - Add your pins here
// Format: PIN_DEF(name, port, pin_number)
#define PIN_TABLE \
    PIN_DEF(TICK,  E, 1) \
    PIN_DEF(DEBUG, E, 2) \
    PIN_DEF(ENABLE, B, 1) \
    PIN_DEF(EVOUT_GATED, B, 2) \
    PIN_DEF(ENABLE_SYNC, B, 3) \
    PIN_DEF(LOOP,  F, 2) \
    PIN_DEF(LED,   F, 3) \
    PIN_DEF(MUX_OUT, A, 3) \
    PIN_DEF(Q_STATE, D, 3) \
    PIN_DEF(CLK_ADC, C, 0) \
    PIN_DEF(PWM1_ADC, C, 1) \
    PIN_DEF(PWM2_ADC, C, 2) \
    PIN_DEF(GATED_CLK, C, 3) \
    PIN_DEF(CLK1_ADC, C, 4)


// Generate TOGGLE macros
#define PIN_DEF(name, port, pin) \
    static inline void name##_TOGGLE(void) { PORT##port.OUTTGL = PIN##pin##_bm; }
PIN_TABLE
#undef PIN_DEF

// Generate SET macros
#define PIN_DEF(name, port, pin) \
    static inline void name##_SET(void) { PORT##port.OUTSET = PIN##pin##_bm; }
PIN_TABLE
#undef PIN_DEF

// Generate CLR macros
#define PIN_DEF(name, port, pin) \
    static inline void name##_CLR(void) { PORT##port.OUTCLR = PIN##pin##_bm; }
PIN_TABLE
#undef PIN_DEF


// Generate init_pins() automatically
static inline void init_pins(void) {

    // Auto-generated from PIN_TABLE
    #define PIN_DEF(name, port, pin) PORT##port.DIRSET = PIN##pin##_bm;
    PIN_TABLE
    #undef PIN_DEF

    // PA2 is POS_EXT input (leave as input).
}
