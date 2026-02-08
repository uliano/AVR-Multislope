/*
 * globals.h
 *
 * Created: 11/3/2022 5:01:41 PM
 *  Author: uliano
 */ 

#pragma once

#include <uart.hpp>

extern Uart<2, UART_ALTERNATE> serial;	

extern volatile uint8_t window_ready;

union WindowPositive {
    uint32_t value;
    uint16_t words[2];
    uint8_t bytes[4];
};



extern volatile WindowPositive window_positive;