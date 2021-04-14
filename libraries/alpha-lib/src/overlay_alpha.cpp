#include <alpha-lib/overlay_alpha.h>
#include <alpha-lib/overlay_alpha.hpp>

#include "SIMD.h"
#include "rescale.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>

/**
 * Overlay a single pixel of a foreground image with an alpha channel over one
 * pixel of a background image.
 *
 * This is used as the "readable" documentation version of the code, for the
 * fallback version without NEON intrinsics, and for images with dimensions that
 * are not an multiple of the vector size.
 */
template <RescaleType rescale_type>
static inline void overlay_alpha_1(const uint8_t *bg_img, const uint8_t *fg_img,
                                   uint8_t *out_img) {
    // Byte order: Blue, Green, Red, Alpha
    // Alpha [0, 255]
    uint16_t alpha = fg_img[3];
    // Complement of Alpha [0, 255]
    uint16_t alpha_c = 255 - alpha;
    // 255 * Red out =
    //     Red foreground * Alpha foreground
    //   + Red background * (255 - Alpha foreground)
    uint16_t r = fg_img[2] * alpha + bg_img[2] * alpha_c;
    out_img[2] = rescale<rescale_type>(r);
    uint16_t g = fg_img[1] * alpha + bg_img[1] * alpha_c;
    out_img[1] = rescale<rescale_type>(g);
    uint16_t b = fg_img[0] * alpha + bg_img[0] * alpha_c;
    out_img[0] = rescale<rescale_type>(b);
    // Alpha channel is not blended, Alpha background is simply copied to output
    out_img[3] = bg_img[3];
}

/**
 * Overlay 8 pixels of a foreground image with an alpha channel over 8 pixels
 * of a background image.
 */
template <RescaleType rescale_type>
static void overlay_alpha_8(const uint8_t *bg_img, const uint8_t *fg_img,
                            uint8_t *out_img);

#if ENABLE_NEON // Fast vectorized version

template <RescaleType rescale_type>
static void overlay_alpha_8(const uint8_t *bg_img, const uint8_t *fg_img,
                            uint8_t *out_img) {
    // Load the four channels of 8 pixels of the foreground image into four
    // uint8x8 vector registers
    uint8x8x4_t fg = vld4_u8(fg_img);
    // Same for the background image
    uint8x8x4_t bg = vld4_u8(bg_img);

    // Byte order: Blue, Green, Red, Alpha
    uint8x8_t alpha   = fg.val[3];
    uint8x8_t alpha_c = vmvn_u8(alpha); // 255 - alpha

    // r = bg.r * (255 - alpha) + fg.r * alpha
    uint16x8_t r = vaddq_u16(vmull_u8(bg.val[2], alpha_c), //
                             vmull_u8(fg.val[2], alpha));

    uint16x8_t g = vaddq_u16(vmull_u8(bg.val[1], alpha_c), //
                             vmull_u8(fg.val[1], alpha));

    uint16x8_t b = vaddq_u16(vmull_u8(bg.val[0], alpha_c), //
                             vmull_u8(fg.val[0], alpha));

    // Divide the 16-bit colors by 255 so the result is in [0, 255] again
    bg.val[2] = rescale<rescale_type>(r);
    bg.val[1] = rescale<rescale_type>(g);
    bg.val[0] = rescale<rescale_type>(b);

    // Store the four channels of 8 pixels to the output image
    vst4_u8(out_img, bg);
}

#else // Fallback without NEON

template <RescaleType rescale_type>
static void overlay_alpha_8(const uint8_t *bg_img, const uint8_t *fg_img,
                            uint8_t *out_img) {

    // Unroll the loop, and inform the compiler that there are no dependencies
    // between loop iterations, so it's free to use SIMD instructions as it sees
    // fit.
#pragma GCC unroll 8
#pragma GCC ivdep
    for (uint8_t i = 0; i < 8; ++i)
        overlay_alpha_1<rescale_type>(&bg_img[4 * i], &fg_img[4 * i],
                                      &out_img[4 * i]);
}

#endif

template <RescaleType rescale_type>
void overlay_alpha_fast(const uint8_t *bg_img, const uint8_t *fg_img,
                        uint8_t *out_img, size_t n) {
    // This fast version assumes that the number of pixels is a multiple of 8,
    // and that the size of the foreground and background images are the same.
    assert(n % 8 == 0);
#pragma omp parallel for
    for (size_t i = 0; i < n * 4; i += 4 * 8)
        overlay_alpha_8<rescale_type>(&bg_img[i], &fg_img[i], &out_img[i]);
}

// Explicit template instantiations
template void overlay_alpha_fast<RescaleType::Div255_Round>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img, size_t n);
template void overlay_alpha_fast<RescaleType::Div255_Round_Approx>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img, size_t n);
template void overlay_alpha_fast<RescaleType::Div255_Floor>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img, size_t n);
template void overlay_alpha_fast<RescaleType::Div256_Round>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img, size_t n);
template void overlay_alpha_fast<RescaleType::Div256_Floor>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img, size_t n);

template <RescaleType rescale_type>
void overlay_alpha_stride(const uint8_t *bg_img, const uint8_t *fg_img,
                          uint8_t *out_img, size_t bg_full_cols, size_t fg_rows,
                          size_t fg_cols, size_t fg_full_cols) {
    // In this case, the number of pixels doesn't need to be a multiple of 8,
    // and by using the right strides, the foreground and background images can
    // have different sizes.
    const size_t fg_rem_cols = fg_cols % 8;
#pragma omp parallel for
    for (size_t r = 0; r < fg_rows; ++r) {
        if (fg_cols >= 8) {
            // Main loop to handle multiples of 8 pixels
            for (size_t c = 0; c < fg_cols - 7; c += 8) {
                size_t bg_offset = 4 * (r * bg_full_cols + c);
                size_t fg_offset = 4 * (r * fg_full_cols + c);
                overlay_alpha_8<rescale_type>(&bg_img[bg_offset],
                                              &fg_img[fg_offset],
                                              &out_img[bg_offset]);
            }
        }
        // Handle the remaining columns (< 8)
        if (fg_rem_cols != 0) {
            size_t bg_offset = 4 * (r * bg_full_cols + fg_cols - fg_rem_cols);
            size_t fg_offset = 4 * (r * fg_full_cols + fg_cols - fg_rem_cols);
            uint8_t tmp_bg[4 * 8];
            uint8_t tmp_fg[4 * 8];
            uint8_t tmp_out[4 * 8];
            memcpy(tmp_bg, &bg_img[bg_offset], 4 * fg_rem_cols);
            memcpy(tmp_fg, &fg_img[fg_offset], 4 * fg_rem_cols);
            overlay_alpha_8<rescale_type>(tmp_bg, tmp_fg, tmp_out);
            memcpy(&out_img[bg_offset], tmp_out, 4 * fg_rem_cols);
        }
    }
}

// Explicit template instantiations
template void overlay_alpha_stride<RescaleType::Div255_Round>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img,
    size_t bg_full_cols, size_t fg_rows, size_t fg_cols, size_t fg_full_cols);
template void overlay_alpha_stride<RescaleType::Div255_Round_Approx>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img,
    size_t bg_full_cols, size_t fg_rows, size_t fg_cols, size_t fg_full_cols);
template void overlay_alpha_stride<RescaleType::Div255_Floor>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img,
    size_t bg_full_cols, size_t fg_rows, size_t fg_cols, size_t fg_full_cols);
template void overlay_alpha_stride<RescaleType::Div256_Round>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img,
    size_t bg_full_cols, size_t fg_rows, size_t fg_cols, size_t fg_full_cols);
template void overlay_alpha_stride<RescaleType::Div256_Floor>(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img,
    size_t bg_full_cols, size_t fg_rows, size_t fg_cols, size_t fg_full_cols);

// C wrappers
extern "C" {
void overlay_alpha_stride_div255_round(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols) {
    overlay_alpha_stride<RescaleType::Div255_Round>(
        bg_img, fg_img, out_img, bg_full_cols, fg_rows, fg_cols, fg_full_cols);
}
void overlay_alpha_stride_div255_round_approx(
    const uint8_t *bg_img, const uint8_t *fg_img, uint8_t *out_img,
    size_t bg_full_cols, size_t fg_rows, size_t fg_cols, size_t fg_full_cols) {
    overlay_alpha_stride<RescaleType::Div255_Round_Approx>(
        bg_img, fg_img, out_img, bg_full_cols, fg_rows, fg_cols, fg_full_cols);
}
void overlay_alpha_stride_div255_floor(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols) {
    overlay_alpha_stride<RescaleType::Div255_Floor>(
        bg_img, fg_img, out_img, bg_full_cols, fg_rows, fg_cols, fg_full_cols);
}
void overlay_alpha_stride_div256_round(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols) {
    overlay_alpha_stride<RescaleType::Div256_Round>(
        bg_img, fg_img, out_img, bg_full_cols, fg_rows, fg_cols, fg_full_cols);
}
void overlay_alpha_stride_div256_floor(const uint8_t *bg_img,
                                       const uint8_t *fg_img, uint8_t *out_img,
                                       size_t bg_full_cols, size_t fg_rows,
                                       size_t fg_cols, size_t fg_full_cols) {
    overlay_alpha_stride<RescaleType::Div256_Floor>(
        bg_img, fg_img, out_img, bg_full_cols, fg_rows, fg_cols, fg_full_cols);
}
}
