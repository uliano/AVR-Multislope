
/*
 * globals.cpp
 *
 * Created: 11/3/2022 5:11:00 PM
 *  Author: uliano
 * Revised: 01/9/2026 - Added modN period definition
 */ 

#include <uart.hpp>
#include "globals.h"


Uart<2, UART_ALTERNATE> serial(115200);  //static object initialized by constructor
// Ring<uint8_t, uint8_t, 8> commands;

volatile uint8_t window_ready = 0;
volatile WindowPositive window_positive = {0};

