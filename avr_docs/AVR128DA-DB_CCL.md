# CCL (Configurable Custom Logic) - AVR128DA/DB Reference

## Hardware Overview

The **AVR128DA** and **AVR128DB** microcontrollers feature **6 LUTs** (Look-Up Tables):
- LUT0, LUT1, LUT2, LUT3, LUT4, LUT5
- Each LUT can implement any 3-input logic function (8-bit truth table)
- LUTs can be combined in pairs to create sequencers (DFF, JK-FF, RS latch, D latch)

## Pin Mapping (Package-Dependent)

### Default Pin Assignment (PORTMUX.CCLROUTEA bits clear)

| LUT | 28-pin | 32-pin | 48-pin | 64-pin |
|-----|--------|--------|--------|--------|
| LUT0 | **PA3** | **PA3** | **PA3** | **PA3** |
| LUT1 | **PC3** | **PC3** | **PC3** | **PC3** |
| LUT2 | **PD3** | **PD3** | **PD3** | **PD3** |
| LUT3 | **PF3** | **PF3** | **PF3** | **PF3** |
| LUT4 | N/A | N/A | N/A | **PB3** |
| LUT5 | N/A | N/A | N/A | **PG3** |

**Important**: LUT4 and LUT5 can operate WITHOUT physical output pins (generating events only) in packages lacking PORTB/PORTG.

### Alternate Pin Assignment

Consult the datasheet "Port Multiplexer" section for alternate pin mapping options. Use `PORTMUX.CCLROUTEA` to select alternate pins.

## Port Availability by Package

| Package | Available Ports |
|---------|-----------------|
| 28-pin | PA, PC, PD, PF (no PB, no PE, no PG) |
| 32-pin | PA, PC, PD, PF (no PB, no PE, no PG) |
| 48-pin | PA, PC, PD, PE, PF (no PB, no PG) |
| 64-pin | PA, PB, PC, PD, PE, PF, PG |

## LUT Configuration

### Input Selection

Each LUT has 3 configurable inputs via `LUTn.CTRLB` (IN0, IN1) and `LUTn.CTRLC` (IN2):

| INSEL | Source | Notes |
|-------|--------|-------|
| `MASK` | Always 1 | Disables input |
| `FEEDBACK` | LUT output | For latches/memory |
| `LINK` | Previous LUT output | Only if sequencer enabled (paired) |
| `EVENTA` | Event system A | Channel configured via EVSYS.USERCCLLUTnA |
| `EVENTB` | Event system B | Channel configured via EVSYS.USERCCLLUTnB |
| `IO` | Physical pin | LUT-specific pin (see datasheet) |
| `AC0` | Analog Comparator 0 | IN0 only |
| `AC1` | Analog Comparator 1 | IN0 only (if available) |
| `AC2` | Analog Comparator 2 | IN0 only (if available) |
| `TCBn` | Timer/Counter B | IN0 only |
| `TCAn` | Timer/Counter A | IN0 only |
| `USARTn` | USART XCK | IN0 only |
| `SPI0` | SPI SCK | IN0 only |

**Important**: AC0/AC1/AC2 are available only on **IN0**, not IN1 or IN2.

### Truth Table (LUTn.TRUTH)

The 8-bit truth table defines output for all input combinations:

```
Bit | IN2 IN1 IN0 | Output
----|-------------|-------
 0  |  0   0   0  | TRUTH[0]
 1  |  0   0   1  | TRUTH[1]
 2  |  0   1   0  | TRUTH[2]
 3  |  0   1   1  | TRUTH[3]
 4  |  1   0   0  | TRUTH[4]
 5  |  1   0   1  | TRUTH[5]
 6  |  1   1   0  | TRUTH[6]
 7  |  1   1   1  | TRUTH[7]
```

**Common patterns**:
- `0xAA` (10101010): Buffer/pass-through = IN0
- `0x55` (01010101): Inverter = !IN0
- `0x08` (00001000): 2-input AND = IN0 AND IN1
- `0x80` (10000000): 3-input AND = IN0 AND IN1 AND IN2
- `0x02` (00000010): !IN0 AND IN1
- `0x20` (00100000): !IN0 AND IN2 (with IN1=1)
- `0xFF` (11111111): Always 1

### Clock Input (LUTn.CTRLC IN2)

If IN2 is configured as clock (with `CLKSRC` in CTRLA), it can use:
- `CLKSEL_IN2`: Event from EVENTA
- `CLKSEL_CLKPER`: CLK_PER (main system clock)
- `CLKSEL_OSCHF`: High-frequency oscillator
- `CLKSEL_OSC32K`: 32 kHz oscillator

## Sequencer (Pairing LUTs)

Sequencers combine 2 consecutive LUTs to create memory elements:

| Sequencer | LUT Pair | Modes |
|-----------|----------|-------|
| SEQ0 | LUT0 + LUT1 | DFF, JK, RS latch, D latch |
| SEQ1 | LUT2 + LUT3 | DFF, JK, RS latch, D latch |
| SEQ2 | LUT4 + LUT5 | DFF, JK, RS latch, D latch |

### D Flip-Flop (DFF)

```
SEQCTRLn = CCL_SEQSEL_DFF_gc

Even LUT (LUT0/2/4): Provides D input
Odd LUT (LUT1/3/5): Provides G (gate enable)

Clock: Even LUT IN2 (must be configured as EVENTA with clock)
Output: Even LUT output = Q
```

**Example** (LUT2+LUT3 for Q = AC0 sampled on clock):
```c
CCL.SEQCTRL1 = CCL_SEQSEL_DFF_gc;

// LUT2: D input
CCL.LUT2CTRLB = CCL_INSEL0_AC0_gc | CCL_INSEL1_MASK_gc;
CCL.LUT2CTRLC = CCL_INSEL2_EVENTA_gc;  // Clock
CCL.TRUTH2 = 0xAA;  // D = IN0 (AC0)
CCL.LUT2CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;

// LUT3: G input (enable)
CCL.LUT3CTRLB = CCL_INSEL0_MASK_gc | CCL_INSEL1_MASK_gc;
CCL.LUT3CTRLC = CCL_INSEL2_MASK_gc;
CCL.TRUTH3 = 0xFF;  // G = 1 (always enabled)
CCL.LUT3CTRLA = CCL_ENABLE_bm;
```

**IMPORTANT**: When using sequencer DFF with LUT0+LUT1, **you cannot use LUT1 independently**! This creates a hardware conflict.

## Event System Integration

### Event Generators (LUT → Event System)

Each LUT output can generate events:

```c
// Example: LUT2 output → CHANNEL5
EVSYS.CHANNEL5 = EVSYS_CHANNEL5_CCL_LUT2_gc;
```

Event generator values for CCL LUTs:
- `EVSYS_CHANNELx_CCL_LUT0_gc`
- `EVSYS_CHANNELx_CCL_LUT1_gc`
- `EVSYS_CHANNELx_CCL_LUT2_gc`
- `EVSYS_CHANNELx_CCL_LUT3_gc`
- `EVSYS_CHANNELx_CCL_LUT4_gc`
- `EVSYS_CHANNELx_CCL_LUT5_gc`

### Event Users (Event System → LUT)

Each LUT can receive events on EVENTA and EVENTB:

```c
// Example: CHANNEL4 → LUT0 EVENTA (IN2)
EVSYS.USERCCLLUT0A = USERVAL(4);  // USERVAL(ch) = ch + 1

// Example: CHANNEL5 → LUT0 EVENTB (IN1 if configured)
EVSYS.USERCCLLUT0B = USERVAL(5);
```

Available event users:
- `EVSYS.USERCCLLUT0A` / `EVSYS.USERCCLLUT0B`
- `EVSYS.USERCCLLUT1A` / `EVSYS.USERCCLLUT1B`
- `EVSYS.USERCCLLUT2A` / `EVSYS.USERCCLLUT2B`
- `EVSYS.USERCCLLUT3A` / `EVSYS.USERCCLLUT3B`
- `EVSYS.USERCCLLUT4A` / `EVSYS.USERCCLLUT4B`
- `EVSYS.USERCCLLUT5A` / `EVSYS.USERCCLLUT5B`

## Typical Configuration Sequence

```c
// 1. Disable CCL
CCL.CTRLA = 0;

// 2. Configure sequencer if needed
CCL.SEQCTRL0 = CCL_SEQSEL_DISABLE_gc;  // or DFF/JK/RS/DLATCH

// 3. Configure inputs for each LUT
CCL.LUT0CTRLB = CCL_INSEL0_EVENTB_gc | CCL_INSEL1_MASK_gc;
CCL.LUT0CTRLC = CCL_INSEL2_EVENTA_gc;

// 4. Configure truth table
CCL.TRUTH0 = 0x80;  // 3-input AND

// 5. Enable LUT (with/without physical output)
CCL.LUT0CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;  // With physical output
// or
CCL.LUT0CTRLA = CCL_ENABLE_bm;  // Events only, no pin

// 6. Configure pin multiplexing if needed
PORTMUX.CCLROUTEA &= ~PORTMUX_LUT0_bm;  // Default pin

// 7. Enable CCL globally
CCL.CTRLA = CCL_ENABLE_bm;
```

## Known Errata (AVR128DA/DB)

### 1. CCL Module Disable Required for Reconfiguration (128DB Rev A4/A5) ⚠️ CRITICAL

**Issue**: To reconfigure a single LUT, the entire CCL module must be disabled. When writing `CCL.CTRLA = 0`, ALL LUTs are disabled, not just the one you want to reconfigure.

**Impact**: If using multiple LUTs for independent tasks, reconfiguring one LUT temporarily interrupts all others.

**Workaround**: Plan CCL configuration in advance during initialization. Avoid reconfiguring LUTs dynamically during runtime. If necessary, reconfigure all LUTs together.

**Note**: This erratum is present on 128DB but NOT on 128DA. If using 128DA, you can reconfigure individual LUTs without disabling the module.

### 2. Sequencer Conflict

**Issue**: When using sequencer DFF/JK/RS with LUT pair (e.g., LUT0+LUT1), both LUTs in the pair are bound to the sequencer and cannot be used independently.

**Workaround**: Use different pairs. If you need LUT1 independent, use LUT2+LUT3 for the sequencer.

### 3. LUT3 LINK Input Not Functional (28/32-pin only)

**Issue**: On 28-pin and 32-pin devices, the LINK option for LUT3 does not work. Output from LUT0 is not connected as input to LUT3.

**Workaround**: Use Event System to connect LUT0 output to LUT3 input instead of direct LINK.

**Note**: Not applicable to 48-pin and 64-pin packages.

### 4. GPIO Event Edge-Triggered (EVSYS)

**Issue**: Events from GPIO pins (PORTE PIN0, etc.) are **edge-triggered**, not level-triggered. When the pin is static, events are not continuously propagated.

**Workaround**:
- Option 1: Use intermediate LUT to buffer GPIO → generates continuous events
- Option 2: Use TCA PWM (0% or 100% duty) instead of GPIO
- Option 3: External physical loop-back

### 5. CLKPER Initialization

**Issue**: If using `CLKSEL_CLKPER` as clock for sequencer, ensure CLK_PER is stable before enabling CCL.

**Workaround**: Initialize clock system before `init_ccl()`.

### 6. LUT Output Delay

**Issue**: LUT output has propagation delay (~2-5 clock cycles). If using LUT output as input to timers/counters, consider this delay.

**Workaround**: Use phase-shifted clock for counting operations to allow LUT outputs to settle.

## Important Notes

1. **Pin availability**: LUT4 and LUT5 have no available pins in 28/32/48-pin packages, but still function for event generation.

2. **Sequencer pairing**: Beware of conflicts when using sequencers - plan LUT allocation in advance.

3. **Event propagation**: CCL events are synchronous with the configured clock; consider latency.

4. **Truth table calculation**: Use online tools or manual tables to verify logic correctness.

5. **Power consumption**: Each enabled LUT consumes power even if unused - disable unnecessary LUTs.

## Differences: 128DA vs 128DB

| Feature | AVR128DA | AVR128DB |
|---------|----------|----------|
| **LUT Reconfiguration** | ✅ Individual LUT reconfigurable | ❌ Requires global CCL disable |
| **LUT3 LINK (28/32-pin)** | ❌ Not functional | ❌ Not functional |
| **Number of LUTs** | 6 LUTs | 6 LUTs (identical) |
| **Pin Mapping** | Identical | Identical |
| **Sequencers** | 3 sequencers | 3 sequencers (identical) |

**Recommendation**: For maximum runtime flexibility, prefer **AVR128DA** if you need to reconfigure CCL dynamically. If CCL configuration is static, either device works well.

## References

- **AVR128DA Datasheet**: Chapter 13 "Configurable Custom Logic (CCL)"
- **AVR128DB Datasheet**: Chapter 13 "Configurable Custom Logic (CCL)"
- **AVR128DA Errata Sheet**: DS80000882B (Rev. B, 11/2020)
- **AVR128DB Errata Sheet**: DS80000915A (Rev. A, 08/2020)
- **AN2447**: Application Note "Using CCL for Digital Logic"
