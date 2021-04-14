#pragma once

#include <cstddef> // size_t
#include <cstdint> // uint8_t

#include <alpha-lib/rescale_type.hpp>

/// @addtogroup overlay_alpha
/// @{

/**
 * @brief   Fast function to overlay two images of the same size, where the 
 *          number of pixels is a multiple of 8.
 * 
 * This function writes the blended color values of the background and the 
 * foreground to the output image.  
 * The alpha channel of the background image is copied to the output unaltered.
 * 
 * ~~~
 *     out.B = (bg.B × (255 - fg.A) + fg.B × fg.A) / 255  
 *     out.G = (bg.G × (255 - fg.A) + fg.G × fg.A) / 255  
 *     out.R = (bg.R × (255 - fg.A) + fg.R × fg.A) / 255  
 *     out.A = bg.A  
 * ~~~
 * (`R` = red, `G` = green, `B` = blue, `A` = alpha)
 * 
 * By default, the division is rounded. By specifying a different 
 * @p rescale_type, you can disable rounding, use a faster, approximating 
 * rounding division by 255, or approximate even more by using a division by 256
 * instead of 255.
 * 
 * All images should be passed to this function as contiguous arrays of bytes
 * with the colors as [B, G, R, A] or ARGB as a little-Endian 32-bit number.
 * 
 * @param   bg_img
 *          A pointer to the image data of the background image.
 * @param   fg_img
 *          A pointer to the image data of the foreground image.
 * @param   out_img
 *          A pointer to the image data of the output image. May be the same as
 *          @p bg_img.
 * @param   n
 *          The number of pixels. Must be a multiple of eight.
 * @tparam  rescale_type
 *          The rescaling mode to use. See @ref RescaleType.
 */
template <RescaleType rescale_type = RescaleType::Div255_Round>
void overlay_alpha_fast(const uint8_t *bg_img, const uint8_t *fg_img,
                        uint8_t *out_img, size_t n);

/**
 * Overlay a smaller image with an alpha channel over a larger background image.
 * 
 * ~~~
 *     ┌────────────────────┐
 *     │  X────────────┬─┐  │ ┬
 *     │  │ foreground │ │  │ │ fg_rows
 *     │  ├────────────┘ │  │ ┴
 *     │  └──────────────┘  │
 *     └────────────────────┘
 *     ├────────────────────┤   bg_full_cols
 *        ├────────────┤        fg_cols
 *        ├──────────────┤      fg_full_cols
 * ~~~
 * 
 * This function writes the mixed color values of the background and the 
 * foreground to the output image.  
 * The alpha channel of the background image is copied to the output unaltered.
 * 
 * ~~~
 *     out.B = (bg.B × (255 - fg.A) + fg.B × fg.A) / 255  
 *     out.G = (bg.G × (255 - fg.A) + fg.G × fg.A) / 255  
 *     out.R = (bg.R × (255 - fg.A) + fg.R × fg.A) / 255  
 *     out.A = bg.A  
 * ~~~
 * (`R` = red, `G` = green, `B` = blue, `A` = alpha)
 * 
 * By default, the division is rounded. By specifying a different 
 * @p rescale_type, you can disable rounding, use a faster, approximating 
 * rounding division by 255, or approximate even more by using a division by 256
 * instead of 255.
 * 
 * All images should be passed to this function as contiguous arrays of bytes in
 * column major order, with the colors as [B, G, R, A] or ARGB as a 
 * little-Endian 32-bit number.
 * 
 * @param   bg_img
 *          A pointer to the image data of the background image.
 * @param   fg_img
 *          A pointer to the image data of the foreground image.
 * @param   out_img
 *          A pointer to the image data of the output image. May be the same as
 *          @p bg_img.
 * @param   bg_full_cols
 *          The total number of columns of the background and the output image.
 *          Used as the stride/leading dimension.
 * @param   fg_rows
 *          The number of rows of the foreground image to overlay.
 * @param   fg_cols
 *          The number of columns of the foreground image to overlay.
 * @param   fg_full_cols
 *          The total number of columns of the foreground image.
 *          Used as the stride/leading dimension.
 * @tparam  rescale_type
 *          The rescaling mode to use. See @ref RescaleType.
 */
template <RescaleType rescale_type = RescaleType::Div255_Round>
void overlay_alpha_stride(const uint8_t *bg_img, const uint8_t *fg_img,
                          uint8_t *out_img, size_t bg_full_cols, size_t fg_rows,
                          size_t fg_cols, size_t fg_full_cols);

/**
 * @copydoc overlay_alpha_stride()
 * 
 * This overload uses references to a specific pixel in the images.
 *
 * You can use it with the result of `cv::Mat::at<uint8_t[4]>(row, col)`.
 */
inline void overlay_alpha_stride(const uint8_t (&bg_img)[4],
                                 const uint8_t (&fg_img)[4],
                                 uint8_t (&out_img)[4], size_t bg_full_cols,
                                 size_t fg_rows, size_t fg_cols,
                                 size_t fg_full_cols) {
    overlay_alpha_stride(&bg_img[0], &fg_img[0], &out_img[0], bg_full_cols,
                         fg_rows, fg_cols, fg_full_cols);
}

/// @}