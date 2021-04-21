//#define PROFILE_BLIT
void GFXCLASS::blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst,
                     uint16_t y_dst, uint16_t width, uint16_t height) {
#ifdef PROFILE_BLIT
  uint32_t s = micros();
#endif
  if (y_dst == y_src && x_dst > x_src) {
    while (height) {
      memmove(&pixelText(x_dst, y_dst), &pixelText(x_src, y_src),
              width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  } else if ((y_dst > y_src) || (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
    while (height) {
      memcpy(&pixelText(x_dst, y_dst), &pixelText(x_src, y_src),
             width * sizeof(pixel_t));
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      memcpy(&pixelText(x_dst, y_dst), &pixelText(x_src, y_src),
             width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  }
#ifdef PROFILE_BLIT
  uint32_t e = micros();
  if (e - s > 1000)
    printf("blit %d us\n", e - s);
#endif
  cleanCache();
}

#include <alpha-lib/overlay_alpha.h>

#define blit_line_alpha(src_x, src_y, dst_x, dst_y, w) \
  overlay_alpha_stride_div255_round_approx( \
    (uint8_t *)&pixelText(dst_x, dst_y), \
    (uint8_t *)&pixelText(src_x, src_y), \
    (uint8_t *)&pixelText(dst_x, dst_y), \
    w, 1, w, w)

void GFXCLASS::blitRectAlpha(uint16_t x_src, uint16_t y_src, uint16_t x_dst,
                             uint16_t y_dst, uint16_t width, uint16_t height) {
  if (y_dst == y_src && x_dst > x_src) {
    while (height) {
      // XXX: implement this correctly
      blit_line_alpha(x_src, y_src, x_dst, y_dst, width);
      y_dst++;
      y_src++;
      height--;
    }
  } else if ((y_dst > y_src) || (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
    while (height) {
      blit_line_alpha(x_src, y_src, x_dst, y_dst, width);
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      blit_line_alpha(x_src, y_src, x_dst, y_dst, width);
      y_dst++;
      y_src++;
      height--;
    }
  }
  cleanCache();
}
