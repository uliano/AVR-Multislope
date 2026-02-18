#pragma once
#include <avr/io.h>
#include "globals.hpp"

enum class InputSource : uint8_t {
    EXTERNAL = 0,
    REF10 = 1,
    REF5 = 2,
    REF2_5 = 3,
    REF0 = 4,
    REF_2_5 = 5,
    REF_5 = 6,
    REF_10 = 7
};

static inline void set_input_source(InputSource source) {
    uint8_t mask =0x70; // DG408 is connected tp PA4-PA5-PA6
    uint8_t input = static_cast<uint8_t>(source) << 4; 
    PORTA.OUT = (PORTA.OUT & ~mask) | input;
    window_counter.reset(); // start new acquisition ASAP
}
