/*
 * globals.h
 *
 * Created: 11/3/2022 5:01:41 PM
 *  Author: uliano
 */ 

#pragma once

#include <uart.hpp>
#include <ticker.hpp>

extern Uart<2, UART_ALTERNATE> serial;	

// Measurement window results (written in ISR, read in main loop)
extern volatile uint8_t window_ready;
extern volatile uint32_t window_positive;
extern volatile uint32_t window_negative;
extern volatile uint8_t window_start_pending;
