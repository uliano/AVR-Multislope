/*
 * window_counter.hpp
 *
 * - 32-bit Modulo-N window counter (TCB2+TCB3)
 *
 * Created: 01/9/2026
 *  Author: uliano
 * Revised: 11/02/2026 Simplified to follow new hardware
 */

#pragma once
#include <avr/io.h>
#include "ticker.hpp"

// Forward declaration of Globals for ISR access  
// Note: The actual definition of Globals is in globals.hpp which includes this header, 
// so we use a forward declaration here to avoid circular dependencies.
// moreover, for the same reason the declaration Status has been moved to status.h 

#include "status.h"

struct Globals;
extern Globals *globals;

/*
 * 32-bit Modulo-N Event Counter using cascaded TCB2+TCB3
 *
 * Implements a hardware event counter that generates a compare event and interrupt
 * after exactly N counts. 
 *
 * Architecture:
 *   Event -> TCB2 (16-bit LSW) -> cascade -> TCB3 (16-bit MSW) -> Overflow IRQ
 *   according to the grid frequency 50/60 Hz TCB2 counts up to respectively 25/30,
 *   so the window counter period will be only in multiple TCB2
 * 
 * First Cycle is special: the integrator input is disconnected to allow for the ADC 
 *   to sample without interference this is implemented by using the TCB0 set up as one-shot
 *   with its output pin as gate for the integrator input, the TCB0 is started by the 
 *   TCB3 compare evnet and it is clocked with the same clock as TCA0. It counts 64 events 
 *   to match the 375 kHz ADC clock cycle. 
 * 
 */


enum class WindowLength : uint16_t {
  PLC_0_02 = 5,
  PLC_0_1 = 25, 
  PLC_0_2 = 50,
  PLC_0_5 = 125,
  PLC_1 = 250,  
  PLC_2 = 500,
  PLC_5 = 1250,
  PLC_10 = 2500,
  PLC_20 = 5000,
  PLC_50 = 12500,
  PLC_100 = 25000,
  PLC_200 = 50000
};

enum class GridFrequency : uint8_t {
  FREQ_50HZ = 30,
  FREQ_60HZ = 25
};

class WindowCounter {
private:
  uint16_t tcb2_cmp;
  uint16_t tcb3_cmp;
  uint16_t tcb2_reload;
  uint16_t tcb3_reload;
  int32_t period_m;
  TimeStamp time_m;

public:
  WindowCounter(WindowLength window_length=WindowLength::PLC_1, 
                GridFrequency grid_freq=GridFrequency::FREQ_50HZ)  {
    tcb2_cmp = static_cast<uint16_t>(grid_freq) -1u;
    tcb2_reload = tcb2_cmp - 1u;  // Reload to one less than compare to trigger on next count
    set_window_length(window_length);
   
    // Configure TCB0 for one-shot mode to disconnect integrator input during first cycle
    TCB0.CTRLA = TCB_CLKSEL_TCA0_gc;
    TCB0.CTRLB = TCB_CNTMODE_SINGLE_gc;  // single shot mode and async to start ASAP after event arrives 
    TCB0.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB0.CNT = 0;
    // this needs to be checked with a scope as the event triggering may lose some cycles 
    TCB0.CCMP = 63;  // Count 64 events to match the 375 kHz ADC clock cycle

    // Configure TCB2 for event counting (will trigger TCB3 on compare via event system)
    TCB2.CTRLB = 0;  // count events
    TCB2.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB2.INTCTRL  = 0;  // Disable capture interrupt on TCB2
    TCB2.CTRLA = TCB_CLKSEL_EVENT_gc;  // Event mode

    // Configure TCB3 for event counting 
    // (on compare will generate ISR and trigger vie event ADC and TCB0)
    TCB3.CTRLB = 0;  // count events
    TCB3.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB3.INTCTRL  = TCB_CAPT_bm;  // Enable capture interrupt on TCB3
    TCB3.INTFLAGS = TCB_CAPT_bm;  // Clear any pending interrupt
    TCB3.CTRLA = TCB_CLKSEL_EVENT_gc;  // Event mode
    set_period();
  }

private:
  inline void set_period(void){
    period_m = static_cast<int32_t>(tcb2_cmp + 1u) * (static_cast<int32_t>(tcb3_cmp) + 1u);
    reset();
  }


public:

  inline void stop(void) {
    TCB0.CTRLA &= ~TCB_ENABLE_bm;
    TCB2.CTRLA &= ~TCB_ENABLE_bm;
    TCB3.CTRLA &= ~TCB_ENABLE_bm;
  }

  inline void start(void) {
    TCB0.CTRLA |= TCB_ENABLE_bm;
    TCB2.CTRLA |= TCB_ENABLE_bm;
    TCB3.CTRLA |= TCB_ENABLE_bm;
  }

  inline void set_window_length(const WindowLength new_length) {
    tcb3_cmp = static_cast<uint16_t>(new_length) -1u;
    tcb3_reload = tcb3_cmp - 1u;  // Reload to one less than compare to trigger on next count
    TCB3.CCMP = tcb3_cmp;
    set_period();
  }

  void isr(void);

  void reset(void);

  int32_t period(void) {
    return period_m;
  }
};



