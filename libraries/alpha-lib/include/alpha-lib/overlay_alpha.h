#pragma once

#ifdef __cplusplus
#include <cstddef> // size_t
#include <cstdint> // uint8_t 
extern "C" {
#else
#include <stddef.h> // size_t
#include <stdint.h> // uint8_t
#endif

/// @addtogroup overlay_alpha_c
/// @{

/// C wrapper for @ref overlay_alpha_stride().
void overlay_alpha_stride_div255_round(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols);
/// C wrapper for @ref overlay_alpha_stride().
void overlay_alpha_stride_div255_round_approx(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img,
    size_t bg_full_cols, size_t fg_rows, size_t fg_cols, size_t fg_full_cols);
/// C wrapper for @ref overlay_alpha_stride().
void overlay_alpha_stride_div255_floor(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols);
/// C wrapper for @ref overlay_alpha_stride().
void overlay_alpha_stride_div256_round(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols);
/// C wrapper for @ref overlay_alpha_stride().
void overlay_alpha_stride_div256_floor(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols);

/// @}

#ifdef __cplusplus
} // extern "C"
#endif