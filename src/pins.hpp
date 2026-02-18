/*
 * pins.hpp
 *
 * Created: 01/9/2026
 *  Author: uliano
 * Revised: 01/30/2026 - Converted from X-Macro to C++ templates
 * Revised: 01/31/2026 - New hardware
 */

#pragma once
#include <avr/io.h>
#include <pin.hpp>

using INT_GATE      = Pin<'A', 2>; // to control the 4053 state, Inverted logic
using REF_POS_GATE  = Pin<'A', 3>; // PWM for POS reference, output of LUT0
using A0            = Pin<'A', 4>; // ADC input selction
using A1            = Pin<'A', 5>; // ADC input selction
using A2            = Pin<'A', 6>; // ADC input selction
using IN_GATE       = Pin<'A', 7>; // Vin gate to Integrator.
using TRG_IN        = Pin<'B', 1>; // Trigger input.
using TRG_OUT       = Pin<'B', 2>; // Trigger output.
using REF_NEG_GATE  = Pin<'B', 3>; // PWM for NEG reference, output of LUT4
using DBG_WOA       = Pin<'B', 4>; // TCD0 WOA output
using DBG_WOB       = Pin<'B', 5>; // TCD0 WOB output
using DBG_CLK_ADC   = Pin<'C', 0>; // TCA0 WO0, Heartbeat clock for ADC sampling
using DBG_WO1       = Pin<'C', 1>; // TCA0 WO1
using DBG_EVT_WO2   = Pin<'C', 2>; // redirect here throught events or TCA0 WO2
using DBG_NEG_CLK   = Pin<'C', 3>; // Negative count, output of LUT1

// PD0 may not be avaliable on AVRxxDB

using INT_OUT       = Pin<'D', 4>; // Analog Input for both ADC and AC1
// PD7 VREF for ADC

using AC_SENSE      = Pin<'E', 3>; // Input for ZC0

// PF4 and PF5 UART2

using LED           = Pin<'F', 2>; // will become PB0

static inline void init_pins(void) {
    INT_GATE::output();
    INT_GATE::invert(true);
    INT_GATE::clear();
    REF_POS_GATE::output();
    REF_NEG_GATE::output();
    A0::output();
    A1::output();   
    A2::output();
    IN_GATE::output();
    TRG_IN::input();
    TRG_OUT::output();
    DBG_WOA::output();
    DBG_WOB::output();
    DBG_CLK_ADC::output();
    DBG_WO1::output();  
    DBG_EVT_WO2::output();
    DBG_NEG_CLK::output();
    INT_OUT::disableDigitalInput();
    AC_SENSE::disableDigitalInput();

    LED::output();

    // limit slew rate on all pins
    PORTA.PORTCTRL |= PORT_SRL_bm;
    PORTB.PORTCTRL |= PORT_SRL_bm;
    PORTC.PORTCTRL |= PORT_SRL_bm;
    PORTD.PORTCTRL |= PORT_SRL_bm;
    PORTE.PORTCTRL |= PORT_SRL_bm;
    PORTF.PORTCTRL |= PORT_SRL_bm;
}
