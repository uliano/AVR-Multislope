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
- avr_docs/: AVR128DA/DB peripheral reference and errata

