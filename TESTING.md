# Testing Plan - Current Hardware Logic

This plan is focused on the current clock, CCL, EVSYS, and counter chain.

## 1) Clock and PWM
- Scope PC0 (375 kHz) and verify duty ~50 percent.
- Optional: scope PC1 and PC2 to verify PWM1 and PWM2.

## 2) Comparator and K sync
- Vary VC (PD2) across VREF (PD0).
- Scope PD3 (K) and confirm it changes only on clock edges.

## 3) ENABLE sync
- Toggle PB1 in software.
- Scope PB3 (ENABLE_SYNC) and confirm it updates only on clock edges.

## 4) Gated clock
- Scope PC3 (GATED_CLK).
- With ENABLE low: PC3 stays low.
- With ENABLE high: PC3 follows PC0 while ENABLE_SYNC is high.

## 5) External AND (POS_EXT)
- Verify external wiring: PD3 AND PC3 -> PA2.
- Scope PA2 (POS_EXT) and confirm pulses only when K=1 and ENABLE_SYNC=1.

## 6) modN window
- Use PF2 or PC4 toggles to confirm TCB3/TCB2 overflow timing.
- Confirm ISR repeats at the expected rate for the chosen N.

## 7) POS count
- Read POS from serial output after each window.
- Expect POS in range [0..N], monotonic with VC around VREF.

## 8) Debug signal set (logic analyzer)
- PC0 (clock)
- PC3 (gated clock)
- PD3 (K)
- PA2 (POS_EXT)
- PB3 (ENABLE_SYNC)
- PB2 (EVOUTB)
- PF2 (ISR toggle)
- PF3 (LED)
