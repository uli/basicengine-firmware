#pragma once

/// @addtogroup rescale255
/// @{

/**
 * @brief   How to carry out the rescaling after performing the alpha overlay.
 * 
 * The product has to be divided by 255 to be exact, but other 
 * divisions can be used for (small) performance gains.
 */
enum class RescaleType {
    /// Exact rounding division by 255.
    /// See @ref div255_round(uint16x8_t) "div255_round"
    Div255_Round,
    /// Approximate rounding division by 255, correct in 99.806% of the cases.
    /// See @ref div255_round_approx(uint16x8_t) "div255_round_approx"
    Div255_Round_Approx,
    /// Exact flooring division by 255.
    /// See @ref div255_floor(uint16x8_t) "div255_floor"
    Div255_Floor,
    /// Fast, rounding division by 256.
    /// See @ref div256_round(uint16x8_t) "div256_round"
    Div256_Round,
    /// Fast, flooring division by 256.
    /// See @ref div256_floor(uint16x8_t) "div256_floor"
    Div256_Floor,
};

/// @}