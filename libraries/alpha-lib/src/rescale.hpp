#pragma once

#include <alpha-lib/rescale_type.hpp>

#include "SIMD.h"

/// @addtogroup rescale255
/// @{

#if ENABLE_NEON

/**
 * This is a flooring division by 256, which is close enough to 255, but the
 * result may be one bit too small.
 */
inline uint8x8_t div256_floor(uint16x8_t x) { return vshrn_n_u16(x, 8); }

/**
 * This is a rounding division by 256, which is close enough to 255, but the
 * result may be one bit too small.
 */
inline uint8x8_t div256_round(uint16x8_t x) {
    return vrshrn_n_u16(x, 8);
}

/*
 * There are some significant differences in how the vector registers are layed
 * out in ARMv8 AArch64. There are also some new NEON instructions that aren't
 * available in AArch32 mode.
 */
#ifdef __ARM_ARCH_ISA_A64 // ARMv8 A64

/**
 * This is an exact flooring division by 255, this is the correct divisor,
 * but requires a little bit more code, and doesn't round the result.
 */
inline uint8x8_t div255_floor(uint16x8_t x) {
    // Multiply by 0x8081 as 32-bit integers (high and low elements separately)
    // 256×256×128/0x8081 ≃ 255
    uint32x4_t h = vmull_high_n_u16(x, 0x8081);
    uint32x4_t l = vmull_n_u16(vget_low_u16(x), 0x8081);
    // Extract the 16 high bits of all 32-bit products (division by 0x10000)
    x = vuzp2q_u16(vreinterpretq_u16_u32(l), vreinterpretq_u16_u32(h));
    // Divide by 0x80 and narrow from 16 bits to 8 bits
    return vshrn_n_u16(x, 7);
}

/**
 * This is an approximation of a rounding division by 255, this is the
 * correct divisor, and it rounds the result correctly 99.80621337890625% of 
 * the time.
 * That is, the result is one unit too large in 127 out of all 2¹⁶ possible 
 * input values.
 * 
 * This function is just as cheap as the @ref div255_floor function.
 */
inline uint8x8_t div255_round_approx(uint16x8_t x) {
    // Multiply by 0x8081 as 32-bit integers (high and low elements separately)
    // 256×256×128/0x8081 ≃ 255
    uint32x4_t h = vmull_high_n_u16(x, 0x8081);
    uint32x4_t l = vmull_n_u16(vget_low_u16(x), 0x8081);
    // Extract the 16 high bits of all 32-bit products (division by 0x10000)
    x = vuzp2q_u16(vreinterpretq_u16_u32(l), vreinterpretq_u16_u32(h));
    // Divide by 0x80 and narrow from 16 bits to 8 bits (with rounding)
    return vrshrn_n_u16(x, 7);
}

/**
 * This is an exact rounding division by 255, this is the correct divisor,
 * and the result is rounded correctly in 100% of the cases.
 * (Compared to `std::round(x / 255.0)`)
 * 
 * The result is incorrect for very large numbers where the quotient would 
 * overflow an 8-bit integer.
 */
inline uint8x8_t div255_round(uint16x8_t x) {
    // Add the rounding constant
    x = vaddq_u16(x, vdupq_n_u16(1 << 7));
    // Multiply by 0x8080 as 32-bit integers (high and low elements separately)
    uint32x4_t h = vmull_high_n_u16(x, 0x8080);
    uint32x4_t l = vmull_n_u16(vget_low_u16(x), 0x8080);
    // Extract the 16 high bits of all 32-bit products (division by 0x10000)
    x = vuzp2q_u16(vreinterpretq_u16_u32(l), vreinterpretq_u16_u32(h));
    // Divide by 0x80 and narrow from 16 bits to 8 bits
    return vshrn_n_u16(x, 7);
}

#else // ARMv8 A32 or ARMv7 NEON

/**
 * This is an exact flooring division by 255, this is the correct divisor,
 * but requires a little bit more code, and doesn't round the result.
 */
inline uint8x8_t div255_floor(uint16x8_t x) {
    // Multiply by 0x8081 as 32-bit integers (high and low elements separately)
    // 256×256×128/0x8081 ≃ 255
    uint32x4_t h = vmull_n_u16(vget_high_u16(x), 0x8081);
    uint32x4_t l = vmull_n_u16(vget_low_u16(x), 0x8081);
    // Extract the 16 high bits of all 32-bit products (division by 0x10000)
    x = vuzpq_u16(vreinterpretq_u16_u32(l), vreinterpretq_u16_u32(h)).val[1];
    // Divide by 0x80 and narrow from 16 bits to 8 bits
    return vshrn_n_u16(x, 7);
}

/**
 * This is an approximation of a rounding division by 255, this is the
 * correct divisor, and it rounds the result correctly 99.80621337890625% of 
 * the time.
 * That is, the result is one unit too large in 127 out of all 2¹⁶ possible 
 * input values.
 * 
 * This function is just as cheap as the @ref div255_floor function.
 */
inline uint8x8_t div255_round_approx(uint16x8_t x) {
    // Multiply by 0x8081 as 32-bit integers (high and low elements separately)
    // 256×256×128/0x8081 ≃ 255
    uint32x4_t h = vmull_n_u16(vget_high_u16(x), 0x8081);
    uint32x4_t l = vmull_n_u16(vget_low_u16(x), 0x8081);
    // Extract the 16 high bits of all 32-bit products (division by 0x10000)
    x = vuzpq_u16(vreinterpretq_u16_u32(l), vreinterpretq_u16_u32(h)).val[1];
    // Divide by 0x80 and narrow from 16 bits to 8 bits
    return vrshrn_n_u16(x, 7);
}

/**
 * This is an exact rounding division by 255, this is the correct divisor,
 * and the result is rounded correctly in 100% of the cases.
 * (Compared to `std::round(x / 255.0)`)
 * 
 * The result is incorrect for very large numbers where the quotient would 
 * overflow an 8-bit integer.
 */
inline uint8x8_t div255_round(uint16x8_t x) {
    // Add the rounding constant
    x = vaddq_u16(x, vdupq_n_u16(1 << 7));
    // Multiply by 0x8080 as 32-bit integers (high and low elements separately)
    uint32x4_t h = vmull_n_u16(vget_high_u16(x), 0x8080);
    uint32x4_t l = vmull_n_u16(vget_low_u16(x), 0x8080);
    // Extract the 16 high bits of all 32-bit products (division by 0x10000)
    x = vuzpq_u16(vreinterpretq_u16_u32(l), vreinterpretq_u16_u32(h)).val[1];
    // Divide by 0x80 and narrow from 16 bits to 8 bits
    return vshrn_n_u16(x, 7);
}

#endif

#endif // NEON

/**
 * This is a flooring division by 256, which is close enough to 255, but the
 * result may be one bit too small.
 */
inline uint8_t div256_floor(uint16_t x) { return x >> 8; }

/**
 * This is a rounding division by 256, which is close enough to 255, but the
 * result may be one bit too small.
 */
inline uint8_t div256_round(uint16_t x) { return (x + (1 << 7)) >> 8; }

/**
 * This is an exact flooring division by 255, this is the correct divisor,
 * but requires a little bit more code, and doesn't round the result.
 */
inline uint8_t div255_floor(uint16_t x) {
    uint32_t h = uint32_t(x) * 0x8081;
    return h >> 23;
}

/**
 * This is an exact rounding division by 255, this is the correct divisor,
 * and the result is rounded correctly in 100% of the cases.
 * (Compared to `std::round(x / 255.0)`)
 */
inline uint8_t div255_round(uint16_t x) {
    x += 1 << 7;
    uint32_t h = uint32_t(x) * 0x101;
    return h >> 16;
}

/**
 * Uses div255_round, and is exact.
 */
inline uint8_t div255_round_approx(uint16_t x) {
    return div255_round(x);
}

// -------------------------------------------------------------------------- //

#if ENABLE_NEON
/**
 * @brief   Rescale the 16-bit color product by dividing by 255 or an 
 *          approximation thereof.
 * @param   x
 *          The 16-bit product to rescale.
 * @tparam  rescale_type
 *          The rescaling mode to use. See @ref RescaleType.
 */
template <RescaleType rescale_type = RescaleType::Div255_Round>
inline uint8x8_t rescale(uint16x8_t x) {
    switch (rescale_type) {
        case RescaleType::Div256_Floor: return div256_floor(x);
        case RescaleType::Div256_Round: return div256_round(x);
        case RescaleType::Div255_Floor: return div255_floor(x);
        case RescaleType::Div255_Round: return div255_round(x);
        case RescaleType::Div255_Round_Approx: return div255_round_approx(x);
        default: return vdup_n_u8(0x00);
    }
}
#endif

/// @copydoc rescale(uint16x8_t)
template <RescaleType rescale_type = RescaleType::Div255_Round>
inline uint8_t rescale(uint16_t x) {
    switch (rescale_type) {
        case RescaleType::Div256_Floor: return div256_floor(x);
        case RescaleType::Div256_Round: return div256_round(x);
        case RescaleType::Div255_Floor: return div255_floor(x);
        case RescaleType::Div255_Round: return div255_round(x);
        case RescaleType::Div255_Round_Approx: return div255_round_approx(x);
        default: return 0x00;
    }
}

/// @}