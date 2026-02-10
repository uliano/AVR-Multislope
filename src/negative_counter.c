#include "negative_counter.h"
uint8_t negative_counter_MSB;

ISR(TCB1_INT_vect) {
    TCB1.INTFLAGS = TCB_OVF_bm;  // Acknowledge overflow
    negative_counter_MSB += 1;
}



