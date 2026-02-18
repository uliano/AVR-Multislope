#pragma once
#include <stdint.h>
/**
 * @brief Convert charge-balance measurement components to a unified 32-bit fixed-point value.
 *
 * This function converts a measurement expressed as:
 *
 *     x = (I + K/D) / J
 *
 * into an unsigned 32-bit fixed-point representation in Q0.32 format:
 *
 *     X = round( x * 2^32 )
 *
 * where:
 *   - I is the integer count of charge injection cycles
 *   - J is the total number of cycles in the acquisition window
 *   - K/D represents the fractional residual charge in the integrator
 *
 * The result X represents a dimensionless fraction in the range [0, 1),
 * with an LSB of 2^-32.
 *
 * --- Assumptions / invariants ---
 * - I, J are uint32_t
 * - K, D are uint16_t
 * - D is a fixed, calibrated constant with: 2048 < D < 4095
 * - 0 <= K < D
 * - J <= 750000
 * - By construction, (I + K/D) < J  ->  x < 1
 *
 * Under these conditions:
 * - denom = J * D fits in 32 bits
 * - numer = I * D + K fits in 64 bits
 * - All intermediate computations are safe using only uint64_t
 * - No floating-point arithmetic is used
 * - No precomputation or calibration constants are required
 *
 * --- Numerical properties ---
 * - The conversion is monotonic and linear
 * - Quantization error is <= 0.5 LSB (Q0.32)
 * - The representation preserves all physically meaningful resolution;
 *   excess lower bits may be discarded later based on stability analysis
 *
 * --- Saturation behavior ---
 * If numer >= denom (i.e. x >= 1 due to unexpected input or transient),
 * the function saturates to 0xFFFFFFFF, corresponding to the maximum
 * representable value just below 1.0.
 *
 * @param I  Number of positive charge injection cycles (integer part)
 * @param K  Fractional residual charge numerator
 * @param J  Total number of cycles in the acquisition window
 * @param D  Fractional charge denominator (calibrated constant)
 *
 * @return Unsigned 32-bit fixed-point value X in Q0.32 format
 */
static inline uint32_t pack_q0_32(uint32_t I, uint16_t K,
                                  uint32_t J, uint16_t D)
{
    uint32_t denom = (uint32_t)(J * (uint32_t)D);
    uint64_t numer = (uint64_t)I * (uint32_t)D + (uint32_t)K;

    if (numer >= (uint64_t)denom)
        return 0xFFFFFFFFu;

    uint64_t num_scaled = (numer << 32) + (uint64_t)(denom / 2u);
    return (uint32_t)(num_scaled / denom);
}
