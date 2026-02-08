/*
 * pins.h
 *
 * Created: 01/9/2026
 *  Author: uliano
 * Revised: 01/30/2026 - Converted from X-Macro to C++ templates
 * Revised: 01/31/2026 - New hardware
 */

#pragma once
#include <avr/io.h>
#include <pin.hpp>

using ENABLE        = Pin<'A', 2>; // to control the 4053 state, Inverted logic
using PWM_POS       = Pin<'A', 3>; // selected PWM for 4053 switches
using PWM_NEG       = Pin<'B', 3>; // PWM negative switch

using CLK_ADC       = Pin<'C', 0>; // for debug only
using WO1           = Pin<'C', 1>; // for debug only
using WO2           = Pin<'C', 2>; // for debug only
using NEG_CLK       = Pin<'C', 3>; // for debug only

// PD0 may not be avaliable on AVRxxDB

using POS_INP       = Pin<'D', 2>; // Analog Input for both ADC and AC1
using AC_SYNC       = Pin<'D', 3>; // for debug only
using NEG_INP       = Pin<'D', 5>; // Analog Input for both ADC and AC1

// PD7 VREF for ADC

// PF0 and PF1 UART2

using DEBUG         = Pin<'F', 2>;
using LED           = Pin<'F', 3>;



static inline void init_pins(void) {
    ENABLE::output();
    ENABLE::invert(true);
    ENABLE::clear();
    PWM_POS::output();
    PWM_NEG::output();

    CLK_ADC::output();
    WO1::output();
    WO2::output();
    NEG_CLK::output();

    POS_INP::disableDigitalInput();
    AC_SYNC::output();
    NEG_INP::disableDigitalInput();

    DEBUG::output();
    LED::output();

    // limit slew rate on all pins
    PORTA.PORTCTRL |= PORT_SRL_bm;
    PORTB.PORTCTRL |= PORT_SRL_bm;
    PORTC.PORTCTRL |= PORT_SRL_bm;
    PORTD.PORTCTRL |= PORT_SRL_bm;
}
