#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include "globals.hpp"

union Word32 {
    int32_t value;
    uint16_t words[2];
    uint8_t bytes[4];
};

class NegativeCounter {
    private:
        uint8_t msb;    
    public:
        NegativeCounter() {
            // Configure TCB1 for event counting (will trigger on negative pulse events)
            TCB1.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
            TCB1.INTCTRL = TCB_OVF_bm;  // Enable overflow interrupt to handle MSB
            TCB1.INTFLAGS = TCB_OVF_bm; // Clear any pending interrupt
            TCB1.CTRLA = TCB_CLKSEL_EVENT_gc;  // EVENT mode 
            reset();
        }  

        inline void reset(void) {
            TCB1.CNT = 0;
            msb = 0;
        }   

        inline void stop(void) {
            TCB1.CTRLA &= ~TCB_ENABLE_bm;
        }

        inline void start(void) {
            TCB1.CTRLA |= TCB_ENABLE_bm;
        } 
        
        inline int32_t get_count(void) {
            Word32 count;
            count.words[0] = TCB1.CNT; 
            count.bytes[2] = msb;  
            return count.value;
        }

        inline void isr(void) {
            TCB1.INTFLAGS = TCB_OVF_bm;  // Acknowledge overflow
            msb += 1;
        }


};
