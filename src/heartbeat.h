#pragma once
#include <avr/io.h>

// TCD is the system heartbeat.
// All integration cycles and windows are defined by TCD compare events.
// TCA/TCB run exclusively in event-counting mode.


static inline void init_heartbeat(void){
    // Disable before changing enable-protected fields
    TCD0.CTRLA = 0;

    // Waveform mode: One Ramp
    TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;   // WGMODE=0x0 (One Ramp)

    // Compare values (12-bit effective, but regs are 16-bit)
    // Period = CMPBCLR + 1 => 64 ticks => CMPBCLR = 63
    // this constraints implies the quite counter intuitive move of the 
    // rising edge of the heartbeat to count 8, on the other hand as 
    // the remaining timers  and ccl only base their counting/clocking 
    // on pulse events it really doesn't matter where the 0 is. 

    TCD0.CMPASET = 8;
    TCD0.CMPACLR = 15;
    TCD0.CMPBSET = 8;
    TCD0.CMPBCLR = 63;

    // Enable compare waveforms
    // FAULTCTRL is CCP-protected.
    // CMPAEN/CMPBEN: enable compare waveforms.
    _PROTECTED_WRITE(TCD0.FAULTCTRL, TCD_CMPAEN_bm | TCD_CMPBEN_bm);

    // Wait until TCD domain is ready
    while (!(TCD0.STATUS & TCD_ENRDY_bm)) { ; }

    // Clock: CLK_PER, prescaler = 1, no prescaler on counter 

    TCD0.CTRLA = TCD_CLKSEL_CLKPER_gc     
              | TCD_CNTPRES_DIV1_gc
              | TCD_ENABLE_bm;
}