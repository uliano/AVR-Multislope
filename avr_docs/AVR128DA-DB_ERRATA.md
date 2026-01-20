# AVR128DA/DB Silicon Errata - Unified Document

## Introduction

This document unifies the errata for **AVR128DA** and **AVR128DB** to facilitate comparison and identification of differences between the two chips.

**Sources**:
- AVR128DA Errata: DS80000882B (Rev. B, 11/2020)
- AVR128DB Errata: DS80000915A (Rev. A, 08/2020)

**Silicon Revisions**:
- AVR128DA: Rev. A6, A7, A8
- AVR128DB: Rev. A4, A5

---

## Legend

- ✅ = Erratum not applicable (or fixed in that revision)
- ❌ = Erratum present
- 🔶 = Applies only to certain revisions

---

## Device - Global Configuration

### 1. Some Reserved Fuse Bits Are '1'

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ❌ | ❌ | ✅ | **Fixed in A8** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: Default fuse values do not conform to datasheet.

**Values read (128DA/DB Rev A6/A4)**:
```
BODCFG = 0x10  (128DB only)
OSCCFG = 0x78  (device uses OSCHF clock)
SYSCFG0 = 0xF2 (128DA) / 0xF6 (128DB)
SYSCFG1 = 0xF8 (128DA) / 0xE8 (128DB)
```

**Workaround**: None needed - values are functional.

---

### 2. CRC Check During Reset Initialization Not Functional

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ❌ | ❌ | ✅ | **Fixed in A8** |
| **128DB** | N/A | N/A | N/A | **Not present** |

**Problem**: CRCSRC bit in SYSCFG0 fuse is ignored during reset. CRC check available only from software.

**Workaround**: None.

---

## CCL - Configurable Custom Logic

### 1. The Entire CCL Module Needs to be Disabled to Change Configuration of a Single LUT

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ✅ | ✅ | ✅ | **NOT present** |
| **128DB** | ❌ | ❌ | N/A | **Present on DB** |

**Problem**: To reconfigure ONE LUT, you must disable the entire CCL module (`CCL.CTRLA = 0`), which disables ALL LUTs.

**Impact**: Temporarily interrupts all LUTs if using CCL for independent tasks.

**Workaround**: Plan CCL configuration during initialization. Avoid dynamic runtime reconfiguration.

**⚠️ PROJECT IMPACT**: Not applicable - we use 128DA and static configuration.

---

### 2. The LINK Input Source Selection for LUT3 Is Not Functional (28/32-pin only)

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ❌ | ❌ | ❌ | **Present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: LINK option for LUT3 does not work on 28-pin and 32-pin devices. LUT0 output not connected to LUT3 input.

**Workaround**: Use Event System to connect LUT0 → LUT3.

**⚠️ PROJECT IMPACT**: NOT applicable (we use 48-pin).

---

## EVSYS - Event System

### 1. Port Pins PB[7:6] and PE[7:4] Are Not Connected to Event System

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ❌ | ❌ | ❌ | **Present** |
| **128DB** | N/A | N/A | N/A | **Not documented** |

**Problem**: Pins PB[7:6] and PE[7:4] are not connected to EVSYS (neither input nor output).

**Impact**: Cannot use events from PE4, PE5, PE6, PE7.

**Workaround**: Use other available pins.

**⚠️ PROJECT IMPACT**: NOT applicable (we use PE1-PE2; PE0 is free).

---

### 2. GPIO Events Are Edge-Triggered (Not Level-Triggered)

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | ❌ | **Present** |

**Problem**: Events from GPIO pins generate events only on **edges**, NOT on static levels. When pin is static, event is generated ONCE.

**Impact**: Static control signals (like ENABLE) do not propagate continuous events.

**Workaround**:
1. Use LUT buffer: `CCL.LUTn CTRLB = CCL_INSEL0_IO_gc` (GPIO direct, not event)
2. Use TCA PWM: 0%/100% duty generates continuous OVF events
3. External physical loop-back

**⚠️ PROJECT CRITICAL**: Known issue, documented in AVR128DA-DB_EVENTS.md. ENABLE uses IO level on PA2/PC2 (CH4 free).

---

## NVMCTRL - Nonvolatile Memory Controller

### 1. Flash Mapping Into Data Space Not Working Properly

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ❌ | ❌ | ❌ | **Present** |
| **128DB** | N/A | N/A | N/A | **Not present** |

**Problem**: Inter-section Flash protection does not consider FLMAP bit. Causes mirroring of BOOT area in every Flash section (32 KB blocks).

**Workaround**: Use only SPM instructions for write and LPM for Flash read.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use Flash mapping).

---

## PORT - I/O Configuration

### 1. Digital Input on Pin Automatically Disabled When Pin Selected for Analog Input

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: If pin is selected for analog input, digital input is automatically disabled.

**Workaround**: None.

**⚠️ PROJECT IMPACT**: NOT relevant (analog pins used only for analog).

---

### 2. PD0 Input Buffer is Floating (128DB only)

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | N/A | N/A | N/A | **Not present** |
| **128DB** | ❌ | ❌ | N/A | **Present on DB** |

**Problem**: On 28-pin and 32-pin, PD0 input buffer is floating. Causes unexpected current consumption (default = input).

**Workaround**:
- Disable PD0 input: `PORTD.PIN0CTRL |= PORT_ISC_INPUT_DISABLE_gc`
- Or configure as output: `PORTD.DIR |= PIN0_bm`

**⚠️ PROJECT IMPACT**: NOT applicable (we use 48-pin + 128DA).

---

## RSTCTRL - Reset Controller

### 1. BOD Registers Not Reset When UPDI Is Enabled

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ❌ | ❌ | ❌ | **Present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: If UPDI is enabled, VLMCTRL, INTCTRL, INTFLAGS registers in BOD are not reset by sources other than POR.

**Workaround**: None.

**⚠️ PROJECT IMPACT**: NOT relevant (we do not use BOD interrupts).

---

## SPI - Serial Peripheral Interface

### 1. SSD Bit Must Be Set When SPIROUTE Value = NONE

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: When PORTMUX.SPIROUTE = NONE, SS pin must be disabled (`CTRLB.SSD = 1`) to maintain Host mode.

**Workaround**: None (required behavior).

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use SPI).

---

## TCA - 16-Bit Timer/Counter Type A

### 1. TCA1 Pinout Alternative 2 and 3 Not Functional

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: Cannot configure TCA1 with pinout alternative 2 or 3 via PORTMUX.TCAROUTEA.

**Workaround**: Use TCA1 pinout alternative 0 or 1.

**⚠️ PROJECT IMPACT**: We use alternative 0 (PORTC) - OK.

---

### 2. Restart Will Reset Counter Direction in NORMAL and FRQ Mode

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: RESTART command or Restart event resets counter direction to default (upwards) when TCA in NORMAL or FRQ mode.

**Workaround**: None.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use RESTART).

---

## TCB - 16-Bit Timer/Counter Type B

### 1. CCMP and CNT Registers Operate as 16-Bit Registers in 8-Bit PWM Mode

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: In 8-bit PWM mode, CNT and CCMP registers operate as 16-bit. Low/high bytes cannot be read/written independently.

**Workaround**: Use 16-bit access to registers.

**⚠️ PROJECT IMPACT**: NOT applicable (we use TCB in event counting mode).

---

## TCD - 12-Bit Timer/Counter Type D

### 1. Asynchronous Input Events Not Working When TCD Counter Prescaler Is Used

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: Asynchronous events (`CFG = 0x2`) + TCD Counter Prescaler (`CNTPRES ≠ 0x0`) → events can be lost.

**Workaround**: Use TCD Synchronization Prescaler (`SYNCPRES`) instead of Counter Prescaler. Or use synchronous events.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use TCD).

---

### 2. CMPAEN Controls All WOx for Alternative Pin Functions

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: When TCD alternative pins are enabled (`PORTMUX.TCDROUTEA ≠ 0x0`), all waveform outputs (WOx) are controlled by CMPAEN.

**Workaround**: None.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use TCD).

---

## TWI - Two-Wire Interface

### 1. The Output Pin Override Does Not Function as Expected

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | ❌ | **Present** |

**Problem**: When TWI is enabled, it overrides pin driver but NOT output value. Output is always HIGH when `PORTx.OUT = 1` for SDA/SCL pins.

**Workaround**: Ensure `PORTx.OUT = 0` for SDA/SCL pins before enabling TWI.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use TWI).

---

### 2. The 50 ns and 300 ns SDA Hold Time Selection Bits Are Swapped

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | N/A | N/A | **Not documented** |

**Problem**: SDAHOLD bits in TWIn.CTRLA are swapped (50ns ↔ 300ns).

**Workaround**: Use 50ns bit for 300ns and vice versa.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use TWI).

---

## USART - Universal Synchronous/Asynchronous Receiver Transmitter

### 1. Open-Drain Mode Does Not Work When TXD Is Configured as Output

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | ❌ | **Present** |

**Problem**: TXD pin configured as output can drive HIGH even with Open-Drain mode enabled.

**Workaround**: Configure TXD as input (`PORTx.DIR = 0`) when using Open-Drain mode.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use Open-Drain).

---

### 2. Start-of-Frame Detection Can Unintentionally Be Enabled in Active Mode When RXCIF Is '0'

| Chip | All Revisions | Status |
|------|---------------|--------|
| **128DA** | ❌ | **Present** |
| **128DB** | ❌ | **Present** |

**Problem**: Start-of-Frame Detector is unintentionally enabled in Active mode when RXCIF = 0. If you read RXDATA while receiving, RXCIF is cleared → SOF detector is activated → frame corrupted.

**Workaround**: Disable SOF Detection (`SFDEN = 0`) in Active mode. Re-enable before Standby sleep mode.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use SOF detection).

---

## ZCD - Zero-Cross Detector

### 1. All ZCD Output Selection Bits Are Tied to the ZCD0 Bit

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | ❌ | ❌ | ❌ | **Present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: ZCD1 and ZCD2 bits in PORTMUX.ZCDROUTEA are tied to ZCD0. Writing to ZCD0 is reflected in ZCD1/ZCD2. Writing to ZCD1/ZCD2 has no effect.

**Workaround**: Use Event System or CCL to route ZCD1/ZCD2 output to pins.

**⚠️ PROJECT IMPACT**: NOT applicable (we do not use ZCD).

---

## CLKCTRL - Clock System (128DB only)

### 1. External Clock/Crystal Status Bit Not Set When External Clock Source Is Ready

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | N/A | N/A | N/A | **Not present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: If external clock is selected (`SELHF = 1`) + RUNSTDBY = 1 without clock request, EXTS bit is not set when clock is ready.

**Workaround**: Request clock from RTC or TCD before checking EXTS bit.

**⚠️ PROJECT IMPACT**: NOT applicable (we use 128DA).

---

### 2. RUNSTDBY Is Not Functional When Using External Clock Sources

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | N/A | N/A | N/A | **Not present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: RUNSTDBY bit in XOSC32KCTRLA does not force external oscillator to remain ON during sleep mode.

**Workaround**: Enable peripheral with external oscillator as clock source to keep it active.

**⚠️ PROJECT IMPACT**: NOT applicable (we use 128DA + 24MHz external clock handled correctly).

---

## ADC - Analog to Digital Converter (128DB only)

### 1. Increased Offset in Single-Ended Mode

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | N/A | N/A | N/A | **Not present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: ADC has typical offset of -3 mV (VDD=3.0V, T=25°C) in single-ended mode.

**Impact**:
- Offset drift vs VDD: -0.3 mV/V
- Offset drift vs temp: -0.02 mV/°C

**Workaround**: Use ADC in differential mode with negative input connected to external GND.

**⚠️ PROJECT IMPACT**: NOT applicable (we use 128DA).

---

## OPAMP - Analog Signal Conditioning (128DB only)

### 1. OPAMP Consume More Power Than Expected

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | N/A | N/A | N/A | **Not present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: OPAMP consumes up to 3x specified current when output is near rails (upper/lower).

**Workaround**: None.

**⚠️ PROJECT IMPACT**: NOT applicable (we use 128DA, no OPAMP).

---

### 2. The Input Range Select is Read-Only

| Chip | Rev. A6/A4 | Rev. A7/A5 | Rev. A8 | Status |
|------|------------|------------|---------|--------|
| **128DA** | N/A | N/A | N/A | **Not present** |
| **128DB** | ❌ | ✅ | N/A | **Fixed in A5** |

**Problem**: IRSEL bit is read-only. Input voltage range is always rail-to-rail when OPAMP is active.

**Workaround**: None.

**⚠️ PROJECT IMPACT**: NOT applicable (we use 128DA, no OPAMP).

---

## Project Impact Summary

### ✅ Non-Relevant Errata (Do Not Impact Project)

1. **CRC Check** - We do not use CRC
2. **Flash Mapping** - We do not use Flash in data space
3. **PD0 Floating (DB)** - We use 128DA + 48-pin
4. **SPI** - We do not use SPI
5. **TCA1 Alternative 2/3** - We use alternative 0
6. **TCA Restart** - We do not use RESTART
7. **TCB 8-bit PWM** - We use event counting
8. **TCD** - We do not use TCD
9. **TWI** - We do not use TWI
10. **USART Open-Drain/SOF** - We do not use these features
11. **ZCD** - We do not use ZCD
12. **CLKCTRL (DB)** - We use 128DA
13. **ADC Offset (DB)** - We use 128DA
14. **OPAMP (DB)** - We use 128DA, no OPAMP
15. **LUT3 LINK 28/32-pin** - We use 48-pin
16. **PE[7:4] Events (DA)** - We use PE1-PE2; PE0 is free

### ⚠️ Relevant Errata (Documented/Handled)

1. **GPIO Edge-Triggered Events** - Documented in AVR128DA-DB_EVENTS.md, CH4 free (ENABLE on IO)
2. **CCL Module Disable (DB)** - Not applicable (128DA + static config)
3. **BOD Registers Reset** - Not relevant (no BOD interrupt)
4. **Fuse Bits Default** - Not relevant (values are functional)

### 🎯 Conclusion

**The firmware is FULLY COMPATIBLE** with both AVR128DA and AVR128DB (recent revisions):
- No critical erratum impacts the project
- Known errata are documented and handled
- Static CCL configuration → no problem with DB reconfiguration limit

---

## References

- **AVR128DA Datasheet**: DS40002183
- **AVR128DB Datasheet**: DS40002247
- **AVR128DA Errata Sheet**: DS80000882B (Rev. B, 11/2020)
- **AVR128DB Errata Sheet**: DS80000915A (Rev. A, 08/2020)
- **[AVR128DA-DB_CCL.md](AVR128DA-DB_CCL.md)**: Detailed CCL errata (this directory)
- **[AVR128DA-DB_EVENTS.md](AVR128DA-DB_EVENTS.md)**: Detailed EVSYS errata (this directory)
- **[SYSTEM_DESIGN.md](../SYSTEM_DESIGN.md)**: Project architecture (root directory)
