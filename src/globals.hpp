/*
 * globals.h
 *
 * Created: 11/3/2022 5:01:41 PM
 *  Author: uliano
 */ 

#pragma once

#include <ring.hpp>
#include <uart.hpp>
#include "negative_counter.hpp"
#include "window_counter.hpp"
#include "status.h"
#include "measurement.hpp"

// C++ objects with static storage, initialized before main() starts.
extern WindowCounter window_counter;  
extern NegativeCounter negative_counter;  
extern Uart<2, UART_ALTERNATE> usb;	
extern Uart<4, UART_STANDARD> console;
extern Ring<Measurement, uint16_t, 1024> meas_buffer; 

// Global variables are 'globbed' :-) into one struct.
struct Globals {
    volatile int16_t previous_charge;
    volatile int16_t charge_difference;
    volatile int32_t negative_counts;
    volatile Status status;
};
extern Globals *globals;

