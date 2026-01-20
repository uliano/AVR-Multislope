# AVR128DA/DB Generic Documentation

This directory contains generic reference documentation for AVR128DA and AVR128DB microcontrollers. These documents are reusable across different projects using these chips.

## Contents

### [AVR128DA-DB_CCL.md](AVR128DA-DB_CCL.md)
Complete reference for the Configurable Custom Logic (CCL) peripheral:
- Hardware overview (6 LUTs, sequencers)
- Pin mapping by package (28/32/48/64-pin)
- LUT configuration (inputs, truth tables, clock sources)
- Sequencer modes (DFF, JK-FF, RS latch, D latch)
- Event system integration
- Known errata and workarounds
- Differences between 128DA and 128DB

### [AVR128DA-DB_EVENTS.md](AVR128DA-DB_EVENTS.md)
Complete reference for the Event System (EVSYS):
- Hardware overview (10 channels, zero CPU overhead)
- Event generators (timers, CCL, ADC, AC, GPIO, etc.)
- Event users (CCL, TCB, ADC, TCD, EVOUT)
- Channel allocation and routing
- Known errata and limitations (GPIO edge-triggered, propagation delay, etc.)
- Best practices and debugging techniques
- Differences between 128DA and 128DB

### [AVR128DA-DB_ERRATA.md](AVR128DA-DB_ERRATA.md)
Unified silicon errata document comparing AVR128DA and AVR128DB:
- Side-by-side comparison tables for all errata
- Impact analysis by silicon revision (A6/A7/A8 for DA, A4/A5 for DB)
- Workarounds for known issues
- Critical issues highlighted
- Organized by peripheral (CCL, EVSYS, TCA, TCB, USART, etc.)

## Usage

These documents are written in English and contain no project-specific information. They serve as:
- Quick reference during development
- Educational material for understanding AVR peripherals
- Troubleshooting guide for common issues
- Comparison tool when choosing between 128DA and 128DB

## Related Project Documentation

For project-specific architecture and implementation details, see:
- [../SYSTEM_DESIGN.md](../SYSTEM_DESIGN.md) - Complete system design for the multislope ADC project
- [../CLAUDE.md](../CLAUDE.md) - Guide for Claude Code when working with this repository
