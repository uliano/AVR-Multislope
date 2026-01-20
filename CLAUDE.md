# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Interaction

Claude will interact with the user in italian but all the edits in the files, including comments, will be done in english.

## Allowed text symbols

Only ASCII <=127 characters will be allowed in the documents.

## Project Overview

This is a multislope ADC hardware front-end on an AVR128DA48 (bare metal). The current focus is a synchronous clocked system that generates K and ENABLE_SYNC, muxes two PWM slopes, and counts positive clock cycles in a fixed window.

Key ideas:
- TCA0 generates a 375 kHz clock (WO0 on PC0) and two PWM slopes (WO1/WO2 on PC1/PC2).
- AC0 (PD2 vs PD0) is sampled by a CCL DFF (LUT2+LUT3) to produce K on PD3.
- ENABLE (PB1, software) is sampled by a CCL DFF (LUT4+LUT5) to produce ENABLE_SYNC on PB3.
- LUT0 muxes WO1/WO2 based on K and outputs to PA3 (MUX_OUT).
- LUT1 gates the clock: ENABLE_SYNC AND WO0 -> PC3 (GATED_CLK).
- External AND gate: K (PD3) AND GATED_CLK (PC3) -> PA2 (POS_EXT).
- TCB0+TCB1 count POS_EXT pulses; TCB2+TCB3 count gated pulses for the modN window.

Full details are in SYSTEM_DESIGN.md.

## Build and Development Commands

### Building
```bash
# Build the project
pio run

# Build and upload via UPDI (default)
pio run -t upload

# Upload and monitor serial output
pio run -t upload && pio device monitor

# Clean build artifacts
pio run -t clean
```

### Serial Monitor
```bash
# Monitor serial output (115200 baud, COM3)
pio device monitor

# Monitor with specific port
pio device monitor -p COM3
```

### Fuses
```bash
# Set fuses (24 MHz internal osc, no bootloader)
pio run -e set_fuses -t fuses

# Read fuses (Python script)
python read_fuses.py

# Write fuses (Python script)
python write_fuses.py
```

### Build Artifacts
- Firmware map file: .pio/build/Upload_UPDI/firmware.map
- Disassembly listing: generated via generate_lst.py post-build script

## Critical Pin Assignments (current)

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
- PD2: VC (AC0 AINP0)
- PD3: K (LUT2 output)

PORT F
- PF2: ISR toggle (TCB3)
- PF3: LED
- PF4/PF5: UART TX/RX
- PF0/PF1: 32 kHz crystal

## Code Organization

Core modules (src/):
- main.cpp: main loop and serial logging
- clocks.h: clock init, TCA0 setup
- switching.h: AC0, CCL, EVSYS routing
- counters.h: TCB counters (POS and modN)
- interrupts.cpp: ISR handlers
- pins.h: pin macros and init
- globals.h/.cpp: globals and UART object
- adc.h: ADC config (not used yet)

Libraries (lib/core/src/):
- uart.hpp: interrupt-driven UART with ring buffers
- ticker.hpp: RTC-based timebase
- timer.hpp: timer helpers
- ring.hpp, utils.hpp: utilities

## Documentation

- SYSTEM_DESIGN.md: current system design and signal flow
- TESTING.md: test plan
- TEST_STATUS.md: short status of verified items
- avr_docs/: AVR128DA/DB peripheral reference and errata

## Constraints and Gotchas

- PA2 port interrupt must remain disabled (PORT_ISC_INTDISABLE_gc) or UART ISR can starve.
- External AND is required to form POS_EXT (K AND gated clock) because LUTs are fully used.
- PA1 is not usable as output on this PCB.
