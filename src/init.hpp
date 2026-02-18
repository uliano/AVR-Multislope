#pragma once
#include "adc.h"
#include "clocks.h"
#include "comparator.h"
#include "events.h"
#include "globals.hpp"
#include "heartbeat.h"
#include "luts.h"
#include "pins.hpp" 
#include "ticker.hpp"
#include "vref.h"

static void init_all(void) {
    ClockInitCode clock_status = init_clocks();

	usb.print("Running on AVR ");
	usb.print(clock_device_family_str(clock_status));
	usb.print("\nClocks:\nmain=");
	usb.print(clock_main_source_str(clock_status));
    if (clock_has_flag(clock_status, ClockInitCode::OschfAutotuned)) {
        usb.print(" (autotuned from XOSC32K)");
    }
    if (clock_has_flag(clock_status, ClockInitCode::HasXosc32k)) {
        usb.print("\nXOSC32K"); 
    } else {
        usb.print("\ninternal OSC32K");
    }
	usb.print("\n");
    init_pins();
    init_ticker();
    init_adc_clock();
    init_vref();
    init_ac1();
    init_adc();
    init_luts();
    init_events();
    // trick the linker allocate meas_buffer.
    // remove when meas_buffer is actually used in the code.
    // Measurement m;
    // meas_buffer.put(m);
    // meas_buffer.get(m);
}

// NegativeCounter and WindowCounter are initialzed in their 
// instantiation as global variables.
