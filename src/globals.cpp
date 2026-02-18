
/*
 * globals.cpp
 *
 * Created: 11/3/2022 5:11:00 PM
 *  Author: uliano
 * Revised: 01/9/2026 - Added modN period definition
 */ 
#include "globals.hpp"

Ring<Measurement, uint16_t, 1024> meas_buffer;

WindowCounter window_counter(WindowLength::PLC_1, GridFrequency::FREQ_50HZ);  
NegativeCounter negative_counter;
Uart<2, UART_ALTERNATE> usb(430200);
Uart<4, UART_STANDARD> console(115200);  // PE0/PE1

Globals global_data = {
    .previous_charge = 0,
    .charge_difference = 0,
    .negative_counts = 0,
    .status = Status::CLEAN
};
Globals *globals = &global_data;  

