/*
 * pins.h
 *
 * Pin assignment using C++ templates
 *
 * To add a new output pin:
 * 1. Add a using alias: using NAME = Pin<'X', N>;
 * 2. Add NAME::output() in init_pins()
 * 3. Use: NAME::toggle(), NAME::set(), NAME::clear()
 *
 * Created: 01/9/2026
 *  Author: uliano
 * Revised: 01/30/2026 - Converted from X-Macro to C++ templates
 */

#pragma once
#include <avr/io.h>

// Template for GPIO pin control
// Uses VPORT for single-cycle SBI/CBI instructions on set/clear/read
// Uses PORT for atomic OUTTGL/DIRSET/DIRCLR operations
// Use invert(true) for active-low signals (hardware inversion via INVEN)
template<char PortLetter, uint8_t PinNum>
struct Pin {
    static_assert(PortLetter >= 'A' && PortLetter <= 'F', "Invalid port");
    static_assert(PinNum <= 7, "Invalid pin number");

    static constexpr uint8_t mask = (1 << PinNum);

    // Regular PORT for atomic set/clear/toggle registers
    static constexpr volatile PORT_t& port() {
        if constexpr (PortLetter == 'A') return PORTA;
        else if constexpr (PortLetter == 'B') return PORTB;
        else if constexpr (PortLetter == 'C') return PORTC;
        else if constexpr (PortLetter == 'D') return PORTD;
        else if constexpr (PortLetter == 'E') return PORTE;
        else return PORTF;
    }

    // Virtual PORT for single-cycle bit access (SBI/CBI)
    static constexpr volatile VPORT_t& vport() {
        if constexpr (PortLetter == 'A') return VPORTA;
        else if constexpr (PortLetter == 'B') return VPORTB;
        else if constexpr (PortLetter == 'C') return VPORTC;
        else if constexpr (PortLetter == 'D') return VPORTD;
        else if constexpr (PortLetter == 'E') return VPORTE;
        else return VPORTF;
    }

    // Access PINnCTRL register for this pin
    static volatile uint8_t& pinctrl() {
        return (&port().PIN0CTRL)[PinNum];
    }

    // Basic I/O
    static void toggle() { port().OUTTGL = mask; }
    static void set()    { vport().OUT |= mask; }
    static void clear()  { vport().OUT &= ~mask; }
    static void output() { port().DIRSET = mask; }
    static void input()  { port().DIRCLR = mask; }
    static bool read()   { return vport().IN & mask; }

    // Hardware inversion (INVEN) - inverts both input and output
    static void invert(bool enable) {
        if (enable) pinctrl() |= PORT_INVEN_bm;
        else pinctrl() &= ~PORT_INVEN_bm;
    }

    // Internal pull-up (only effective when pin is input)
    static void pullup(bool enable) {
        if (enable) pinctrl() |= PORT_PULLUPEN_bm;
        else pinctrl() &= ~PORT_PULLUPEN_bm;
    }

    // Disable digital input buffer (saves power for analog pins)
    static void disableDigitalInput() {
        pinctrl() = (pinctrl() & ~PORT_ISC_gm) | PORT_ISC_INPUT_DISABLE_gc;
    }

    // Enable digital input buffer (default state)
    static void enableDigitalInput() {
        pinctrl() = (pinctrl() & ~PORT_ISC_gm) | PORT_ISC_INTDISABLE_gc;
    }
};

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
// PC0 (OUTPUT PP)  ADC clock 375 kHz (TCA0 WO0, PORTMUX->PORTC)
// PC1 (OUTPUT PP)  PWM (TCA0 WO1, CMP1=7)
// PC2 (OUTPUT PP)  PWM (TCA0 WO2, CMP2=55)
// PC3 (OUTPUT PP)  GATED_CLK (LUT1 output, debug and AND input)
// PC4 (OUTPUT PP)  TCB2 overflow debug (GPIO toggle)

// PORT D
// PD0 (ANALOG IN)  VREF (AC0 negative input, ADC-)
// PD1 (ANALOG IN)  free (unused)
// PD2 (ANALOG IN)  VC (AC0 positive input, ADC+)
// PD3 (OUTPUT PP)  K (CCL LUT2 output, debug)
// PD4 (ANALOG IN)  free (unused)

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

// Pin aliases - add your pins here
using TICK        = Pin<'E', 1>;
using DEBUG       = Pin<'E', 2>;
using ENABLE      = Pin<'B', 1>;
using EVOUT_GATED = Pin<'B', 2>;
using ENABLE_SYNC = Pin<'B', 3>;
using LOOP        = Pin<'F', 2>;
using LED         = Pin<'F', 3>;
using MUX_OUT     = Pin<'A', 3>;
using Q_STATE     = Pin<'D', 3>;
using CLK_ADC     = Pin<'C', 0>;
using PWM1_ADC    = Pin<'C', 1>;
using PWM2_ADC    = Pin<'C', 2>;
using GATED_CLK   = Pin<'C', 3>;
using CLK1_ADC    = Pin<'C', 4>;

static inline void init_pins(void) {
    TICK::output();
    DEBUG::output();
    ENABLE::output();
    EVOUT_GATED::output();
    ENABLE_SYNC::output();
    LOOP::output();
    LED::output();
    MUX_OUT::output();
    Q_STATE::output();
    CLK_ADC::output();
    PWM1_ADC::output();
    PWM2_ADC::output();
    GATED_CLK::output();
    CLK1_ADC::output();
    // PA2 is POS_EXT input (leave as input)
}
