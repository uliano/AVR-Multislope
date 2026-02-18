#pragma once
#include <avr/io.h>

// Event channel assignment.
enum {
    EVENT_HEARTBEAT = 0,   // TCA0 OVF -> LUT0 & LUT1 & LUT2A clock & TCB2 count 
    EVENT_WINDOW_COMPLETE = 1,   // TCB3 OVF -> End of WINDOW
    EVENT_TCB2_OVF = 2, // TCB2 OVF -> TCB3 COUNT
    EVENT_AC_SYNC = 3,   // LUT2 output -> LUT0 select PWM_PATTERN
    EVENT_NEG_CLK = 4,   // LUT1 output -> TCB0 count
};


static inline void init_events(void)
{
    // Configure event channels.

    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_TCA0_OVF_LUNF_gc;
    EVSYS.CHANNEL1 = EVSYS_CHANNEL1_TCB3_CAPT_gc;
    EVSYS.CHANNEL2 = EVSYS_CHANNEL2_TCB2_OVF_gc;
    EVSYS.CHANNEL3 = EVSYS_CHANNEL3_CCL_LUT2_gc;
    EVSYS.CHANNEL4 = EVSYS_CHANNEL4_CCL_LUT1_gc;

    // Route events to users.

    // Heartbeat are counted by WindowCounter 
    EVSYS.USERCCLLUT2A = (uint8_t)(EVENT_HEARTBEAT + 1u);
    // and used to sync the AC in the DFF made by LUT2 & LUT3.
    EVSYS.USERTCB2COUNT = (uint8_t)(EVENT_HEARTBEAT + 1u);

    // AC_SYNC selects the pwm pattern for both positive
    // and negative inputs respectively via LUT0 and LUT4.
    EVSYS.USERCCLLUT0A = (uint8_t)(EVENT_AC_SYNC + 1u);
    EVSYS.USERCCLLUT4A = (uint8_t)(EVENT_AC_SYNC + 1u);
    // It is also used to gate the HEARTBEAT clock to generate 
    // pulses on cycles where the negative input is on.
    EVSYS.USERCCLLUT1A = (uint8_t)(EVENT_AC_SYNC + 1u); 
    
    // Negative pulses are counted by NegativeCounter.
    EVSYS.USERTCB1COUNT = (uint8_t)(EVENT_NEG_CLK + 1u);
    
    // Ripple overflow for the 32bit WindowCounter.
    EVSYS.USERTCB3COUNT = (uint8_t)(EVENT_TCB2_OVF + 1u);

    // On window complete we trigger the ADC and start TCB0 that
    // disconnects the integrator input for the first cycle
    EVSYS.USERADC0START = (uint8_t)(EVENT_WINDOW_COMPLETE + 1u);
    EVSYS.USERTCB0COUNT = (uint8_t)(EVENT_WINDOW_COMPLETE + 1u);
}