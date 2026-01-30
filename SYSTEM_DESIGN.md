# Multislope ADC - Current System Design

This document describes the current, working hardware logic and timing for the AVR128DA48 setup.

## Overview

- MCU: AVR128DA48, bare metal
- Core clock: 24 MHz external on PA0
- ADC clock: 375 kHz from TCA0 (WO0 on PC0)
- Comparator: AC0 (PD2 vs PD0)
- Logic: CCL LUTs used for K sync, ENABLE sync, PWM mux, gated clock
- Counters: TCB0+TCB1 (positive count), TCB2+TCB3 (modN window)
- External AND: K (PD3) AND GATED_CLK (PC3) -> PA2 (POS_EXT)

## Signal Flow (high level)

```
TCA0 WO0 (PC0) ---------+
                         \                 +--> TCB2 (LSW) --> TCB3 (MSW) -> ISR
ENABLE (PB1) -> LUT4 DFF -+-> LUT1 (gated) -+

AC0 (PD2 vs PD0) -> LUT2 DFF -> K (PD3)
K (PD3) ------------------------+ 
                                |   external AND -> PA2 (POS_EXT) -> TCB0 -> TCB1
GATED_CLK (PC3) -----------------+

K (LUT2) selects WO1/WO2 -> LUT0 -> PA3 (MUX_OUT)
```

## Clocks

### System clock
- External 24 MHz on PA0 when present.
- Fallback to internal 24 MHz if external fails.
- 32.768 kHz crystal on PF0/PF1 used for RTC when present.

### TCA0 (375 kHz)
- PER = 63 -> 375 kHz from 24 MHz.
- WO0 -> PC0 (50 percent duty) used as ADC clock and gating source.
- WO1 -> PC1 (CMP1=7, about 1/8 duty).
- WO2 -> PC2 (CMP2=55, about 7/8 duty).
- LUT0 mux selects between WO1 and WO2 to drive PA3.

## Comparator and Synchronizers

### AC0
- Positive input: PD2 (VC).
- Negative input: PD0 (VREF).
- Output feeds LUT2 DFF input.

### K sync (LUT2 + LUT3)
- DFF sampled on TCA0 OVF (event).
- Output K on PD3 (debug and external AND input).

### ENABLE sync (LUT4 + LUT5)
- DFF sampled on TCA0 OVF (event).
- Input ENABLE from PB1 (software output).
- Output ENABLE_SYNC on PB3.

## PWM Mux (LUT0)

- IN0 = K (from LUT2 via EVSYS).
- IN1 = TCA0 WO1.
- IN2 = TCA0 WO2.
- Output on PA3 (MUX_OUT).
- Result: PA3 outputs WO1 when K=1, WO2 when K=0.

## Gated Clock (LUT1)

- IN0 = TCA0 WO0.
- IN1 = ENABLE_SYNC.
- Output on PC3 (GATED_CLK).
- Used as the clock input for modN and for the external AND.

## External AND (POS_EXT)

- Hardware AND: K (PD3) AND GATED_CLK (PC3).
- Output goes to PA2 (POS_EXT input).
- PA2 is routed to EVSYS CH0 and counted by TCB0.
- Port interrupt on PA2 must stay disabled to avoid UART starvation.

## Counters

### modN window (TCB2 + TCB3)
- TCB2 counts gated clock pulses (EVSYS CH5).
- TCB3 counts TCB2 overflows (EVSYS CH6).
- ISR on TCB3 overflow ends the window, latches POS count, reloads, restarts.

### positive count (TCB0 + TCB1)
- TCB0 counts POS_EXT pulses (PA2 via EVSYS CH0).
- TCB1 counts TCB0 overflows (EVSYS CH1).
- Result is a 32-bit count of clock cycles where K=1 inside the enabled window.

## ISR Behavior (TCB3 overflow)

1) Disable ENABLE (PB1 low)
2) Stop modN and positive counters
3) Read 32-bit positive count (TCB1:TCB0)
4) Store result and mark window_ready
5) Reload counters and restart
6) Re-enable ENABLE (PB1 high)

## Event System Mapping

- CH0: PA2 (POS_EXT) -> TCB0 count
- CH1: TCB0 OVF -> TCB1 count
- CH2: TCA0 OVF -> LUT2A and LUT4A (DFF clocks)
- CH3: LUT2 OUT (K) -> LUT0 select
- CH4: LUT4 OUT (ENABLE_SYNC) -> LUT1 gate
- CH5: LUT1 OUT (GATED_CLK) -> TCB2 count, EVOUTB (PB2)
- CH6: TCB2 OVF -> TCB3 count

## Pin Map (current)

PORT A
- PA0: external 24 MHz clock input (do not use as GPIO)
- PA1: not usable on PCB
- PA2: POS_EXT input (external AND output)
- PA3: MUX_OUT (LUT0 output)

PORT B
- PB1: ENABLE (software output)
- PB2: EVOUTB (gated clock pulses)
- PB3: ENABLE_SYNC (LUT4 output)

PORT C
- PC0: CLK_ADC (TCA0 WO0, 375 kHz)
- PC1: PWM1 (TCA0 WO1)
- PC2: PWM2 (TCA0 WO2)
- PC3: GATED_CLK (LUT1 output)
- PC4: debug toggle (TCB2 ISR)

PORT D
- PD0: VREF (AC0 AINN1)
- PD1: free (unused)
- PD2: VC (AC0 AINP0, ADC+)
- PD3: K (LUT2 output)
- PD4: free (unused)

PORT F
- PF2: LOOP debug (TCB3 ISR toggle)
- PF3: LED
- PF4: UART TX
- PF5: UART RX
- PF0/PF1: 32 kHz crystal

## Debug Signals (suggested)

- PC0: raw clock (375 kHz)
- PC3: gated clock (ENABLE_SYNC AND clock)
- PD3: K (synced comparator)
- PA2: POS_EXT (external AND output)
- PB2: EVOUTB (gated clock pulses)
- PB3: ENABLE_SYNC
- PF2: modN ISR toggle
- PF3: LED

## Notes and Constraints

- PA2 must keep PORT interrupt disabled (PORT_ISC_INTDISABLE_gc) or UART ISR can starve.
- External AND is required because LUTs are all in use.
- PA1 cannot be used as output on this PCB.

## Next Step

Connect the integrator and drive AC0 with real data to validate full multislope behavior.
