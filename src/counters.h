/*
 * counters.h
 *
 * Event counters for multislope window:
 * - 32-bit positive counter (TCB0+TCB1)
 * - 32-bit Modulo-N window counter (TCB2+TCB3)
 *
 * Created: 01/9/2026
 *  Author: uliano
 */

#pragma once
#include <avr/io.h>

/*
 * 32-bit Modulo-N Event Counter using cascaded TCB2+TCB3
 *
 * Implements a hardware event counter that generates an overflow interrupt
 * after exactly N events. Uses two 16-bit TCB timers in cascade mode to
 * create a 32-bit counter.
 *
 * Architecture:
 *   Event -> TCB2 (16-bit LSW) -> cascade -> TCB3 (16-bit MSW) -> Overflow IRQ
 *
 * The counter is initialized to (0 - N) so that after N events it reaches
 * 0x00000000, triggering the TCB3 overflow interrupt. This allows precise
 * period measurement or event windowing.
 *
 * Usage:
 *   init_modN();           // Initialize hardware
 *   setup_modN(1000000);   // Set N = 1M events
 *   start_modN();          // Start counting
 *
 *   // In ISR(TCB3_INT_vect):
 *   stop_modN();           // Stop counting
 *   // ... process measurement ...
 *   reload_modN();         // Reset to initial value
 *   start_modN();          // Restart
 */

// Pre-calculated initial values for modN counter (set by setup_modN)
extern volatile uint16_t modN_init_lo;  // LSW of (0 - N)
extern volatile uint16_t modN_init_hi;  // MSW of (0 - N)
extern volatile uint32_t modN_period;   // N (events per window)

/**
 * @brief Initialize modN hardware (TCB2 and TCB3)
 *
 * Disables both timers, enables TCB3 overflow interrupt.
 * Must be called once at startup before using modN counter.
 */
static inline void init_modN(void)
{
    TCB2.CTRLA = 0;  // Stop TCB2 (LSW)
    TCB3.CTRLA = 0;  // Stop TCB3 (MSW)

    // Configure TCB2 for event counting (will trigger TCB3 on overflow via event system)
    TCB2.CTRLB = 0;  // No special mode needed, just count events
    TCB2.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB2.INTCTRL  = TCB_OVF_bm;  // Debug: enable overflow interrupt on TCB2
    TCB2.INTFLAGS = TCB_OVF_bm;  // Clear any pending interrupt

    // Configure TCB3 to count TCB2 overflows via event system
    TCB3.CTRLB = 0;  // No special mode needed
    TCB3.EVCTRL = TCB_CAPTEI_bm;  // Ensure event input is edge-qualified
    TCB3.INTCTRL  = TCB_OVF_bm;  // Enable overflow interrupt on TCB3
    TCB3.INTFLAGS = TCB_OVF_bm;  // Clear any pending interrupt
}

/**
 * @brief Reload counter to initial value (0 - N)
 *
 * Resets both TCB2 and TCB3 to their pre-calculated initial values
 * so that the counter will overflow after exactly N events.
 * Clears any pending overflow interrupt.
 *
 * Call this in the ISR after processing the overflow, or to restart
 * a measurement window.
 */
static inline void reload_modN(void)
{
    TCB2.CNT = modN_init_lo;        // Reload LSW
    TCB3.CNT = modN_init_hi;        // Reload MSW
    TCB3.INTFLAGS = TCB_OVF_bm;     // Clear overflow flag
}

/**
 * @brief Configure modN period and reload counter
 *
 * @param N Number of events before overflow (1 to 2^32-1)
 *
 * Calculates initial counter value as (0 - N) using two's complement,
 * so that after N increments the 32-bit counter reaches 0x00000000
 * and triggers TCB3 overflow interrupt.
 *
 * Example: N=1000 -> init=0xFFFFFC18 -> after 1000 events -> 0x00000000
 *
 * Automatically calls reload_modN() to set the counters.
 */
static inline void setup_modN(uint32_t N)
{
    uint32_t init = 0u - N;              // Two's complement: -N
    modN_init_lo = (uint16_t)init;       // Lower 16 bits
    modN_init_hi = (uint16_t)(init >> 16); // Upper 16 bits
    modN_period = N;
    reload_modN();
}

/**
 * @brief Stop modN counter
 *
 * Disables both TCB2 and TCB3. Counter values are preserved.
 * Typically called in ISR to freeze the counter during measurement processing.
 */
static inline void stop_modN(void)
{
    TCB2.CTRLA &= ~TCB_ENABLE_bm;  // Stop LSW
    TCB3.CTRLA &= ~TCB_ENABLE_bm;  // Stop MSW
}

/**
 * @brief Start modN counter in event mode
 *
 * Configures both timers for event counting (mode 0x7) and enables them.
 * TCB3 must be started first to ensure proper cascade operation.
 *
 * Event source must be configured via event system (see switching.h).
 */
static inline void start_modN(void)
{
    TCB3.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // Event mode + Enable MSW first
    TCB2.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // Event mode + Enable LSW
}

static inline void init_count_positive(void)
{
    // TCB0 counts events on CH_POS (positive pulses)
    TCB0.CNT = 0;
    TCB0.EVCTRL = TCB_CAPTEI_bm;
    TCB0.INTCTRL = 0;  // No ISR; MSW handled by TCB1 via EVSYS
    TCB0.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // EVENT + ENABLE
}

static inline void init_count_positive_msw(void)
{
    // TCB1 counts TCB0 overflows on CH_POS_OVF
    TCB1.CNT = 0;
    TCB1.EVCTRL = TCB_CAPTEI_bm;
    TCB1.INTCTRL = 0;  // No ISR
    TCB1.CTRLA = (0x7 << 1) | TCB_ENABLE_bm;  // EVENT + ENABLE
}

static inline void init_counters(void) {
    init_count_positive();
    init_count_positive_msw();
}

// Read 32-bit counter atomically WITHOUT disabling interrupts
static inline uint32_t read_positive(void)
{
    uint16_t msb1, msb2;
    uint16_t lsw;

    do {
        msb1 = TCB1.CNT;
        lsw = TCB0.CNT;
        msb2 = TCB1.CNT;
    } while (msb1 != msb2);  // Repeat if overflow occurred during read

    return ((uint32_t)msb2 << 16) | lsw;
}

static inline void stop_counters(void)
{
    TCB0.CTRLA &= ~TCB_ENABLE_bm;
    TCB1.CTRLA &= ~TCB_ENABLE_bm;
}

static inline void stop_window_counters(void)
{
    stop_modN();
    stop_counters();
}

static inline void reset_counters(void)
{
    TCB0.CNT = 0;
    TCB1.CNT = 0;
}

static inline void start_counters(void)
{
    TCB0.CTRLA |= TCB_ENABLE_bm;
    TCB1.CTRLA |= TCB_ENABLE_bm;
}
