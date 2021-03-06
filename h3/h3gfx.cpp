// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifdef H3

#include <Arduino.h>
#include <display.h>
#include "h3gfx.h"
#include "colorspace.h"
#include <joystick.h>

H3GFX vs23;

const struct video_mode_t H3GFX::modes_pal[H3_SCREEN_MODES] = {
  { 460, 224, 16, 16, 1 },
  { 436, 216, 16, 16, 1 },
  { 320, 216, 16, 16, 2 },   // VS23 NTSC demo
  { 320, 200, 16, 16, 2 },   // (M)CGA, Commodore et al.
  { 256, 224, 16, 16, 25 },  // SNES
  { 256, 192, 16, 16, 25 },  // MSX, Spectrum, NDS
  { 160, 200, 16, 16, 4 },   // Commodore/PCjr/CPC
                             // multi-color
  // "Overscan modes"
  { 352, 240, 16, 16, 2 },  // PCE overscan (barely)
  { 282, 240, 16, 16, 2 },  // PCE overscan (underscan on PAL)
  { 508, 240, 16, 16, 1 },
  // ESP32GFX modes
  { 320, 256, 16, 16, 2 },  // maximum PAL at 2 clocks per pixel
  { 320, 240, 16, 16, 2 },  // DawnOfAV demo, Mode X
  { 640, 256, 16, 16, 1 },
  // default H3 mode
  { 480, 270, 16, 16, 1 },
};

#include <usb.h>
#include <config.h>
void H3GFX::begin(bool interlace, bool lowpass, uint8_t system) {
  m_display_enabled = false;
  delay(16);
  m_last_line = 0;
  m_pixels = NULL;
  setMode(CONFIG.mode - 1);

  m_bin.Init(0, 0);

  m_frame = 0;
//  m_pal.init();
  reset();

  m_display_enabled = true;
}

void H3GFX::reset() {
  BGEngine::reset();
  for (int i = 0; i < m_last_line; ++i)
    memset(m_pixels[i], 0, m_current_mode.x * 4);
  setColorSpace(0);
}

void H3GFX::blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst,
                     uint16_t y_dst, uint16_t width, uint16_t height) {
  if ((y_dst > y_src) ||
      (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
    while (height) {
      memmove(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width * sizeof(pixel_t));
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      memmove(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  }
}

static uint32_t *backbuffer = 0;

bool H3GFX::setMode(uint8_t mode) {
  m_display_enabled = false;

  free(m_pixels);
  free(backbuffer);

  m_current_mode = modes_pal[mode];

  // Try to allocate no more than 128k, but make sure it's enough to hold
  // the specified resolution plus color memory.
  m_last_line = _max(524288 / m_current_mode.x,
                     m_current_mode.y + m_current_mode.y / MIN_FONT_SIZE_Y);

  m_pixels = (uint32_t **)malloc(sizeof(*m_pixels) * m_last_line);

  backbuffer = (uint32_t *)calloc(sizeof(pixel_t),
                                  (m_current_mode.x + m_current_mode.left * 2) *
                                  (m_last_line + m_current_mode.top));

  for (int i = 0; i < m_last_line; ++i) {
    m_pixels[i] = backbuffer +
                  (m_current_mode.x + m_current_mode.left * 2) *
                          (i + m_current_mode.top) +
                  m_current_mode.left;
  }

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  display_set_mode(m_current_mode.x, m_current_mode.y, m_current_mode.left,
                   m_current_mode.top);

  m_display_enabled = true;

  return true;
}

//#define PROFILE_BG

void H3GFX::updateBg() {
  static uint32_t last_frame = 0;

  if (frame() <= last_frame + m_frameskip)
    return;

  last_frame = frame();

  memcpy((void *)active_buffer, backbuffer,
         (m_current_mode.x + m_current_mode.left * 2) *
         (m_current_mode.y + m_current_mode.top) *
         sizeof(pixel_t));

  buffer_swap();

  if (!m_bg_modified)
    return;

  m_bg_modified = false;

#ifdef PROFILE_BG
  uint32_t start = micros();
#endif
  for (int b = 0; b < MAX_BG; ++b) {
    bg_t *bg = &m_bg[b];
    if (!bg->enabled)
      continue;

    int tsx = bg->tile_size_x;
    int tsy = bg->tile_size_y;

    // start/end coordinates of the visible BG window, relative to the
    // BG's origin, in pixels
    int sx = bg->scroll_x;
    int ex = bg->win_w + bg->scroll_x;
    int sy = bg->scroll_y;
    int ey = bg->win_h + bg->scroll_y;

    // offset to add to BG-relative coordinates to get screen coordinates
    int owx = -sx + bg->win_x;
    int owy = -sy + bg->win_y;

    for (int y = sy; y < ey; ++y) {
      for (int x = sx; x < ex; ++x) {
        int off_x = x % tsx;
        int off_y = y % tsy;
        int tile_x = x / tsx;
        int tile_y = y / tsy;
next:
        uint8_t tile = bg->tiles[tile_x % bg->w + (tile_y % bg->h) * bg->w];
        int t_x = bg->pat_x + (tile % bg->pat_w) * tsx + off_x;
        int t_y = bg->pat_y + (tile / bg->pat_w) * tsy + off_y;
        if (!off_x && x < ex - tsx) {
          // can draw a whole tile line
          memcpy(&m_pixels[y + owy][x + owx], &m_pixels[t_y][t_x], tsx * 4);
          x += tsx;
          tile_x++;
          goto next;
        } else {
          m_pixels[y + owy][x + owx] = m_pixels[t_y][t_x];
        }
      }
    }
  }

#ifdef PROFILE_BG
  uint32_t taken = micros() - start;
  Serial.printf("rend %d\r\n", taken);
#endif

  for (int si = 0; si < MAX_SPRITES; ++si) {
    sprite_t *s = &m_sprite[si];
    // skip if not visible
    if (!s->enabled)
      continue;
    if (s->pos_x + s->p.w < 0 ||
        s->pos_x >= width() ||
        s->pos_y + s->p.h < 0 ||
        s->pos_y >= height())
      continue;

    // consider flipped axes
    int dx, offx;
    if (s->p.flip_x) {
      dx = -1;
      offx = s->p.w - 1;
    } else {
      dx = 1;
      offx = 0;
    }
    int dy, offy;
    if (s->p.flip_y) {
      dy = -1;
      offy = s->p.h - 1;
    } else {
      dy = 1;
      offy = 0;
    }

    // sprite pattern start coordinates
    int px = s->p.pat_x + s->p.frame_x * s->p.w + offx;
    int py = s->p.pat_y + s->p.frame_y * s->p.h + offy;

    for (int y = 0; y != s->p.h; ++y) {
      int yy = y + s->pos_y;
      if (yy < 0 || yy >= height())
        continue;
      for (int x = 0; x != s->p.w; ++x) {
        int xx = x + s->pos_x;
        if (xx < 0 || xx >= width())
          continue;
        pixel_t p = m_pixels[py + y * dy][px + x * dx];
        // draw only non-keyed pixels
        if (p != s->p.key)
          m_pixels[yy][xx] = p;
      }
    }
  }
}

#ifdef USE_BG_ENGINE
uint8_t H3GFX::spriteCollision(uint8_t collidee, uint8_t collider) {
  uint8_t dir = 0x40;  // indicates collision

  const sprite_t *us = &m_sprite[collidee];
  const sprite_t *them = &m_sprite[collider];

  if (us->pos_x + us->p.w < them->pos_x)
    return 0;
  if (them->pos_x + them->p.w < us->pos_x)
    return 0;
  if (us->pos_y + us->p.h < them->pos_y)
    return 0;
  if (them->pos_y + them->p.h < us->pos_y)
    return 0;

  // sprite frame as bounding box; we may want something more flexible...
  const sprite_t *left = us, *right = them;
  if (them->pos_x < us->pos_x) {
    dir |= joyLeft;
    left = them;
    right = us;
  } else if (them->pos_x + them->p.w > us->pos_x + us->p.w)
    dir |= joyRight;

  const sprite_t *upper = us, *lower = them;
  if (them->pos_y < us->pos_y) {
    dir |= joyUp;
    upper = them;
    lower = us;
  } else if (them->pos_y + them->p.h > us->pos_y + us->p.h)
    dir |= joyDown;

  // Check for pixels in overlapping area.
  const int leftpatx = left->p.pat_x + left->p.frame_x * left->p.w;
  const int leftpaty = left->p.pat_y + left->p.frame_y * left->p.h;
  const int rightpatx = right->p.pat_x + right->p.frame_x * right->p.w;
  const int rightpaty = right->p.pat_y + right->p.frame_y * right->p.h;

  for (int y = lower->pos_y;
       y < _min(lower->pos_y + lower->p.h, upper->pos_y + upper->p.h);
       y++) {
    int leftpy = leftpaty + y - left->pos_y;
    int rightpy = rightpaty + y - right->pos_y;

    for (int x = right->pos_x;
         x < _min(right->pos_x + right->p.w, left->pos_x + left->p.w);
         x++) {
      int leftpx = leftpatx + x - left->pos_x;
      int rightpx = rightpatx + x - right->pos_x;
      int leftpixel = m_pixels[leftpy][leftpx];
      int rightpixel = m_pixels[rightpy][rightpx];

      if (leftpixel != left->p.key && rightpixel != right->p.key)
        return dir;
    }
  }

  // no overlapping pixels
  return 0;
}
#endif	// USE_BG_ENGINE

#endif	// H3
