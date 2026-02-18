#pragma once
#include <Ticker.hpp>
#include <ring.hpp> 


struct Measurement {
    uint32_t timestamp;  // ~ milliseconds, roll over in 49 days.
    int32_t value;       
};