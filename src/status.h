#pragma once
#include <stdint.h>

enum class Status : uint8_t {
    CLEAN = 0,
    PREV_CHARGE = 1,
    NEGATIVE_COUNTS = 2,
    RESULT_AVAIL = 3
};


