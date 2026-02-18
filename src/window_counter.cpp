#include "globals.hpp"

// These functions are moved here because they access members of the struct Globals,
// and therefore could not live in the .hpp file (forward declaration only).

void WindowCounter::isr(void) {
    TCB3.INTFLAGS = TCB_CAPT_bm;  // Acknowledge overflow
    uint32_t timestamp = Ticker::ptr->millis();
    globals->previous_charge = globals->charge_difference;
    globals->charge_difference = negative_counter.get_count();
    globals->negative_counts = negative_counter.get_count();
    globals->status = Status::NEGATIVE_COUNTS;  // TODO to be removed once the ISR for ADC is working
}

void WindowCounter::reset(void) {
    TCB0.CNT = 0;
    TCB2.CNT = tcb2_reload;
    TCB3.CNT = tcb3_reload;
    globals->status = Status::CLEAN;
}
