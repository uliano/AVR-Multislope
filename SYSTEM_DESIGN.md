# Multislope ADC - Current System Design

This document describes the current, working hardware logic and timing for the AVR-Multislope setup.

## Overview

- MCU: AVR128DB48, bare metal
- Core clock: 24 MHz external on PA0 or crystal on PA0, PA1
- Heartbeat: 375 kHz from TCA0 64 counts of PER_CLK
    - HB_CLK symmetryc waveform (debug possible on PC0)
    - PWM1 = WO1 on for 8 cycles 
    - PWM2 = WO2 on for 56 cycles
- Comparator: AC1 PD2 vs REFDAC
- ADC: reads the difference PD2 - REFDAC sampling the Integrator during 
    the very first window count triggered by window_counter OVF event
    with 1 ADC_CLK = 500ns as SAMPDLY to let the switch settle.
    - ADC_CLK = CLK_PER / 12 = 2 MHz
- Logic: 
    - PA3 = REF+GATE = AC_SYNC ? PWM1 : PWM2 realized with LUT0 
    - NEG_CLK = AC_SYNC AND HB_CLK realized with LUT1 (debug can be on PC3)
    - AC_SYNC = D Flip Flop syncronizing AC0 to HB_CLK 
      realized with LUTs 2 and 3 (debig possible on PD3)
    - PB3 = REF-GATE = AC_SYNC ? PWM2 : PWM1 relized with LUT 4
    - PA2 = INT_GATE* realized with TCB0 used as a set reset filp flop:
      this signal is used to blank the Integrator inputs sync with the 
      very first cycle of the window counting. The idea is to make a single shot
      clocked on the same clock of TCA0 and counting 64. WO is the signal
- Event counters: 
    - negative_counter 24 bit: 16 LSW by TCB1 -> OVF Interrupt updates MSB
    - window_counter TCB2 as a prescaler > Event OVF -> TCB3 the requirement 
      here is to be able to count more than 16 bits while keeping the ability 
      to generate OVF Interrupt. Initialization should fail on undivisible counts.
      IMPLEMENTATION DETAIL: TCB2 is used only for counts greater than 65535

- Event channels:
    - 0 Heartbeat (TCA0_OVF)
    - 1 Window End (TCA3_CAPT)
    - 2 Window prescaler overflow (TCA2_OVF)
    - 3 AC_SYNC (LUT2)
    - 4 Negative Clock (LUT1)
- Interrupts:
    - TCB1 OVF ripple count to MSB on RAM
    - TCB3 OVF stores past acquisition window negative count to RAM
    - ADC RESRDY computes (and stores to RAM) the difference between current and 
      past value of the residual charge then stores (in RAM) the new value as old
      finally sets the flag that allows superloop compute the voltage from 
      (previously stored) negative_counts and residual charge.

- Superloop
    - I/O to UART, I2C, SPI
    - calibrations
        - statistic sampling of the possible values read by the ADC to
          find MaxADC and MinADC. MaxADC - MinADC will give the 
          Frac_Den denominator of the fractional part. 
        - reference reading to define the slope and intercept of the linear 
          representation of the measurements or, better the Volt per count 
          of the integral part (slope) and the reading at GND intercept


- Arithmentic
    the rapresentaion here is 

    negative_counts + ((residual charge now - residual charge before)/(calibrated denominator))
    -------------------------------------------------------------------------------------------
                                 total_window_counts
    
    to convert to an uint32_t we use:
    
    static inline uint32_t pack_q0_32(uint32_t I, uint16_t K,uint32_t J, uint16_t D)

    from the arithmetic.h file





