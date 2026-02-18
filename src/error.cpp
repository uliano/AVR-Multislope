#include <avr/io.h>
#include <util/delay.h>
#include "pins.hpp"

void error_blink(uint8_t code) {
    while (1) {
        for (uint8_t i = 0; i < code; i++) {
            LED::toggle();
            _delay_ms(200);
        }
        _delay_ms(1000);
    }
}